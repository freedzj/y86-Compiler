#include <string>
#include <cstdint>
#include <iostream>
#include "RegisterFile.h"
#include "PipeRegField.h"
#include "PipeReg.h"
#include "F.h"
#include "D.h"
#include "E.h"
#include "M.h"
#include "W.h"
#include "Stage.h"
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "Memory.h"
#include "Tools.h"
#include "FetchStage.h"
#include "DecodeStage.h"
#include "ExecuteStage.h"

Memory * mem = Memory::getInstance();

/*
 * doClockLow:
 * Performs the Fetch stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool FetchStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    F * freg = (F *) pregs[FREG];
    D * dreg = (D *) pregs[DREG];
    E * ereg = (E *) pregs[EREG];
    M * mreg = (M *) pregs[MREG];
    W * wreg = (W *) pregs[WREG];
    uint64_t f_pc = 0, icode = 0, ifun = 0, valC = 0, valP = 0;
    uint64_t rA = RNONE, rB = RNONE; 

    f_pc = selectPC(freg, mreg, wreg);

    bool error = false;
    if(error)
    {
        ifun = FNONE;
    }
    else
    {
        ifun = Tools::getBits(mem->getByte((uint32_t) f_pc, error), 0, 3);
    }

    if(error)
    {
        icode = INOP;
    }
    else
    {
        icode = Tools::getBits(mem->getByte((uint32_t) f_pc, error), 4, 7);
    }

    uint64_t stat = setf_stat(icode, error);

    bool needRegID = needRegIds(icode, f_pc, error, rA, rB);
    bool nValC = needValC(icode, valC, f_pc, error);
    valP = PCIncrement(f_pc, icode, needRegID, nValC);

    uint64_t prePC = predictPC(icode, valC, valP);

    //The value passed to setInput below will need to be changed
    freg->getpredPC()->setInput(prePC);
    
    calculateControlSignals(dreg, ereg, mreg, stages);

    //provide the input values for the D register
    setDInput(dreg, stat, icode, ifun, rA, rB, valC, valP);
    return false;
}

/* doClockHigh
 * applies the appropriate control signal to the F
 * and D register intances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void FetchStage::doClockHigh(PipeReg ** pregs)
{
    F * freg = (F *) pregs[FREG];
    D * dreg = (D *) pregs[DREG];

    if (!F_stall)
    {
        freg->getpredPC()->normal();
    }
    else
    {
        freg->getpredPC()->stall();
    }

    if (D_bubble)
    {
        bubbleD(dreg);
    }
    else
    {
        if (!D_stall)
        {
            normalD(dreg);
        }
        else
        {
            dreg->getstat()->stall();
            dreg->geticode()->stall();
            dreg->getifun()->stall();
            dreg->getrA()->stall();
            dreg->getrB()->stall();
            dreg->getvalC()->stall();
            dreg->getvalP()->stall();
        }
    }
}

/* setDInput
 * provides the input to potentially be stored in the D register
 * during doClockHigh
 *
 * @param: dreg - pointer to the D register instance
 * @param: stat - value to be stored in the stat pipeline register within D
 * @param: icode - value to be stored in the icode pipeline register within D
 * @param: ifun - value to be stored in the ifun pipeline register within D
 * @param: rA - value to be stored in the rA pipeline register within D
 * @param: rB - value to be stored in the rB pipeline register within D
 * @param: valC - value to be stored in the valC pipeline register within D
 * @param: valP - value to be stored in the valP pipeline register within D
 */
void FetchStage::setDInput(D * dreg, uint64_t stat, uint64_t icode, 
        uint64_t ifun, uint64_t rA, uint64_t rB,
        uint64_t valC, uint64_t valP)
{
    dreg->getstat()->setInput(stat);
    dreg->geticode()->setInput(icode);
    dreg->getifun()->setInput(ifun);
    dreg->getrA()->setInput(rA);
    dreg->getrB()->setInput(rB);
    dreg->getvalC()->setInput(valC);
    dreg->getvalP()->setInput(valP);
}

/**
 * selectPC
 * selects the next pc in fetch stage.
 *
 * @param: freg - pointer to the F register instance.
 * @param: mreg - pointer to the M register instance.
 * @param: wreg - pointer to the W register instance.
 *
 * @return: uint64_t - f_pc.
 */
uint64_t FetchStage::selectPC(F * freg, M * mreg, W * wreg)
{
    uint64_t f_pc;

    if (mreg->geticode()->getOutput() == IJXX && !(mreg->getCnd()->getOutput()))
    {
        f_pc = mreg->getvalA()->getOutput();
    }
    else if (wreg->geticode()->getOutput() == IRET)
    {
        f_pc = wreg->getvalM()->getOutput();
    }
    else
    {
        f_pc = freg->getpredPC()->getOutput();
    }

    return f_pc;
}

/*
 * needRegIds
 * method that returns a bool stating if the register IDs are needed.
 *
 * @param: icode - value from icode pipeline register.
 * @param: f_pc - value from the f_pc value within FetchStage.
 * @param: error - boolean value from FetchStage.
 * @param: rA - value from the rA pipeline register.
 * @param: rB - value from the rB pipeline register.
 */
bool FetchStage::needRegIds(uint64_t icode, uint64_t f_pc, bool & error, uint64_t & rA, uint64_t & rB)
{
    switch(icode) 
    {
        case(IRRMOVQ):
        case(IIRMOVQ):
        case(IRMMOVQ):
        case(IMRMOVQ):
        case(IOPQ):         
        case(IPUSHQ):
        case(IPOPQ):
            rA = Tools::getBits(mem->getByte((uint32_t) f_pc + 1, error), 4, 7);
            rB = Tools::getBits(mem->getByte((uint32_t) f_pc + 1, error), 0, 3);
            return true;
        default:
            return false;
    }
}

/*
 * needValC
 * method that returns a bool stating if an instruction code needs valC.
 *
 * @param: icode - value from icode pipeline register.
 * @param: valC - value from valC pipeline register.
 * @param: f_pc - value from the f_pc value within FetchStage.
 * @param: error - boolean value from FetchStage.
 *
 * @return: bool - states whether or not valC is needed for instruction code.
 */
bool FetchStage::needValC(uint64_t icode, uint64_t & valC, uint64_t f_pc, bool & error)
{
    uint8_t bytes[LONGSIZE];
    switch(icode) {
        case(IIRMOVQ):
        case(IRMMOVQ):
        case(IMRMOVQ):
            for (int i = 0; i < 8; i++)
            {
                bytes[i] = mem->getByte((uint32_t) f_pc + 2 + i, error);
            }
            valC = Tools::buildLong(bytes);
            return true;
        case(IJXX):
        case(ICALL):
            for (int i = 0; i < 8; i++)
            {
                bytes[i] = mem->getByte((uint32_t) f_pc + 1 + i, error);
            }
            valC = Tools::buildLong(bytes);
            return true;
        default:
            return false;
    }
}

/*
 * predictPC
 * method that predicts the next pc.
 *
 * @param: icode - value from the icode pipeline register.
 * @param: valC - value from the valC FetchStage.
 * @param: valP - value from the valP FetchStage.
 *
 * @return: uint64_t - predicted PC.
 */
uint64_t FetchStage::predictPC(uint64_t icode, uint64_t valC, uint64_t valP)
{
    uint64_t predPC;

    switch(icode) {
        case(IJXX):
        case(ICALL):
            predPC = valC;
            break;
        default:
            predPC = valP;
    }

    return predPC;
}

/*
 * PCIncrement
 * method to increment the PC.
 *
 * @param: f_pc - value from f_pc method in FetchStage.
 * @param: icode - value from icode pipeline register.
 * @param: needRegID - value from needRegIds method.
 * @param: needValC - value from needValC method.
 *
 * @returns incremented pc.
 */
uint64_t FetchStage::PCIncrement(uint64_t f_pc, uint64_t icode, bool needRegID, bool needValC)
{
    uint64_t nextAddr = f_pc + 1;

    if (needRegID)
    {
        nextAddr++;
    }

    if (needValC)
    {
        nextAddr += 8;
    }

    return nextAddr;
}

/*
 * instr_valid
 * Method that tells whether an instruction uses the correct icode.
 *
 * @param icode: value from icode pipeline register.
 *
 * @returns true if icode is in a certain instruction, false otherwise.
 */
bool FetchStage::instr_valid(uint64_t icode)
{
    switch(icode)
    {
        case(INOP):
        case(IHALT):
        case(IRRMOVQ):
        case(IIRMOVQ):
        case(IRMMOVQ):
        case(IMRMOVQ):
        case(IOPQ):
        case(IJXX):
        case(ICALL):
        case(IRET):
        case(IPUSHQ):
        case(IPOPQ):
            return true;
            break;
        default:
            return false;
    }
}

/*
 * set_fstat
 * method that sets the stat pipeline register code
 *
 * @param icode: value from icode pipeline register
 * @param error: memory error value
 *
 * @return stat for fetch stage pipeline.
 */
uint64_t FetchStage::setf_stat(uint64_t icode, bool & error)
{
    uint64_t f_stat;
    if(error)
    {
        f_stat = SADR;
    }
    else if(!instr_valid(icode))
    {
        f_stat = SINS;
    }
    else if(icode == IHALT)
    {
        f_stat = SHLT;
    }
    else
    {
        f_stat = SAOK;
    }
    
    return f_stat;
}

/*
 * setF_stall
 * method that determines whether the F register needs to be stalled.
 *
 * @param D_icode - value from Decode Stage icode pipeline register
 * @param E_icode - value from Execute Stage icode pipeline register
 * @param M_icode - value from Memory Stage icode pipeline register
 * @param E_dstM - value from dstM pipeline register in Execute Stage.
 * @param d_srcA - value from Deocde Stage srcA
 * @param d_srcB - value from Decode Stage srcB
 *
 * @return true if F stage needs to be stalled, false otherwise.
 */
bool FetchStage::setF_stall(uint64_t D_icode, uint64_t E_icode, uint64_t M_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB)
{
    if (((E_icode == IMRMOVQ || E_icode == IPOPQ) && (E_dstM == d_srcA || E_dstM == d_srcB))
        || (D_icode == IRET || E_icode == IRET || M_icode == IRET))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/*
 * setD_stall
 * method that determines whether the D register needs to be stalled.
 *
 * @param E_icode - value from Execute Stage icode pipeline register
 * @param E_dstM - value from dstM pipeline register in Execute Stage.
 * @param d_srcA - value from Decode Stage srcA
 * @param d_srcB - value from Decode Stage srcB
 *
 * @return true if D stage needs to be stalled, false otherwise.
 */
bool FetchStage::setD_stall(uint64_t E_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB)
{
    if ((E_icode == IMRMOVQ || E_icode == IPOPQ) && (E_dstM == d_srcA || E_dstM == d_srcB))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/*
 * calculateControlSignals
 * Method that sets F_stall, D_stall, and D_bubble to see if stages need to be stalled or bubbled.
 *
 * @param dreg - D pipe register
 * @param ereg - E pipe register
 * @param mreg - M pipe register
 * @param stages - stages of registers
 */
void FetchStage::calculateControlSignals(D * dreg, E * ereg, M * mreg, Stage ** stages)
{
    uint64_t D_icode = dreg->geticode()->getOutput();

    uint64_t E_icode = ereg->geticode()->getOutput();
    uint64_t E_dstM = ereg->getdstM()->getOutput();

    uint64_t M_icode = mreg->geticode()->getOutput();

    Stage * dec = stages[DREG];
    uint64_t d_srcA = ((DecodeStage *)dec)->getsrcA();
    uint64_t d_srcB = ((DecodeStage *)dec)->getsrcB();

    Stage * exe = stages[EREG];
    uint64_t e_Cnd = ((ExecuteStage *)exe)->getCnd();

    F_stall = setF_stall(D_icode, E_icode, M_icode, E_dstM, d_srcA, d_srcB);
    D_stall = setD_stall(E_icode, E_dstM, d_srcA, d_srcB);
    D_bubble = setD_bubble(D_icode, E_icode, M_icode, e_Cnd, E_dstM, d_srcA, d_srcB);
}

/*
 * setD_bubble
 * method that determines whether or not D register needs to be bubbled.
 *
 * @param D_icode - value from Decode Stage icode pipeline register
 * @param E_icode - value from Execute Stage icode pipeline register
 * @param M_icode - value from Memory Stage icode pipeline register
 * @param e_Cnd - value from Execute Stage Cnd pipeline register
 * @param E_dstM - value from Execute Stage dstM pipeline register
 * @param d_srcA - value from Decode Stage srcA
 * @param d_srcB - value from Decode Stage srcB
 *
 * @return true if D stage needs to be bubbled, false otherwise
 */
bool FetchStage::setD_bubble(uint64_t D_icode, uint64_t E_icode, uint64_t M_icode, uint64_t e_Cnd, uint64_t E_dstM,
    uint64_t d_srcA, uint64_t d_srcB)
{
    return ((E_icode == IJXX && !e_Cnd) || (!((E_icode == IMRMOVQ || E_icode == IPOPQ) && (E_dstM == d_srcA 
        || E_dstM == d_srcB)) && (D_icode == IRET || E_icode == IRET || M_icode == IRET)));
}

/*
 * bubbleD
 * Method that bubbles D if setD_bubble is true.
 *
 * @param dreg - D stage pipe register.
 */
void FetchStage::bubbleD(D * dreg)
{
    dreg->getstat()->bubble(SAOK);
    dreg->geticode()->bubble(INOP);
    dreg->getifun()->bubble();
    dreg->getrA()->bubble(RNONE);
    dreg->getrB()->bubble(RNONE);
    dreg->getvalC()->bubble();
    dreg->getvalP()->bubble();
}

/*
 * normalD
 * Method that sends the F stage pipe register to D stage pipe register.
 *
 * @param dreg - D stage pipe register.
 */
void FetchStage::normalD(D * dreg)
{
    dreg->getstat()->normal();
    dreg->geticode()->normal();
    dreg->getifun()->normal();
    dreg->getrA()->normal();
    dreg->getrB()->normal();
    dreg->getvalC()->normal();
    dreg->getvalP()->normal();
}

