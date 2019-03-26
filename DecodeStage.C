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
#include "Status.h"
#include "Debug.h"
#include "Instructions.h"
#include "DecodeStage.h"
#include "ExecuteStage.h"
#include "MemoryStage.h"

/*
 * doClockLow:
 * Performs the Decode stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool DecodeStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    D * dreg = (D *) pregs[DREG];
    E * ereg = (E *) pregs[EREG];
    M * mreg = (M *) pregs[MREG];
    W * wreg = (W *) pregs[WREG];

    Stage * ex = stages[ESTAGE];
    uint64_t e_valE = ((ExecuteStage *) ex)->gete_valE();
    uint64_t e_dstE = ((ExecuteStage *) ex)->gete_dstE();

    Stage * me = stages[MSTAGE];
    uint64_t m_valM = ((MemoryStage *) me)->getm_valM();

    uint64_t stat = dreg->getstat()->getOutput();
    uint64_t icode = dreg->geticode()->getOutput();
    uint64_t ifun = dreg->getifun()->getOutput();
    uint64_t valC = dreg->getvalC()->getOutput();
    uint64_t valP = dreg->getvalP()->getOutput();
    uint64_t rA = dreg->getrA()->getOutput();
    uint64_t rB = dreg->getrB()->getOutput();

    bool error = false;
    RegisterFile * reg = RegisterFile::getInstance();

    srcA = setSrcA(icode, rA);
    srcB = setSrcB(icode, rB);

    uint64_t dstE = setDstE(icode, rB);
    uint64_t dstM = setDstM(icode, rA);

    uint64_t d_valA = reg->readRegister(srcA, error);
    uint64_t d_valB = reg->readRegister(srcB, error);

    uint64_t valA = setValA(d_valA, srcA, e_dstE, e_valE, icode, valP, m_valM, mreg, wreg);
    uint64_t valB = setValB(d_valB, srcB, e_dstE, e_valE, m_valM, mreg, wreg);
    
    E_bubble = calculateControlSignals(ereg, stages);

    setEInput(ereg, stat, icode, ifun, valC, valA, valB, dstE, dstM, srcA, srcB);

    return false;
}

/* doClockHigh
 * applies the appropriate control signal to the E register instance
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void DecodeStage::doClockHigh(PipeReg ** pregs)
{
    E * ereg = (E *) pregs[EREG];
    
    if (E_bubble)
    {
        bubbleE(ereg);
    }
    else
    {
        normalE(ereg);
    }
}


/* setEInput
 * provides the input to be stored in the E register
 * during doClockHigh
 *
 * @param: ereg - pointer to the E register instance
 * @param: stat - value to be stored in the stat pipeline register within E
 * @param: icode - value to be stored in the icode pipeline register within E
 * @param: ifun - value to be stored in the ifun pipeline register within E
 * @param: valC - value to be stored in the valC pipeline register within E
 * @param: valA - value to be stored in the valA pipeline register within E
 * @param: valB - value to be stored in the valB pipeline register within E
 * @param: dstE - value to be stored in the dstE pipeline register within E
 * @param: dstM - value to be stored in the dstM pipeline register within E
 * @param: srcA - value to be stored in the srcA pipeline register within E
 * @param: srcB - value to be stored in the srcB pipeline register within E
 */
void DecodeStage::setEInput(E * ereg, uint64_t stat, uint64_t icode,
        uint64_t ifun, uint64_t valC, uint64_t valA, uint64_t valB, uint64_t dstE, uint64_t dstM,
        uint64_t srcA, uint64_t srcB)
{
    ereg->getstat()->setInput(stat);
    ereg->geticode()->setInput(icode);
    ereg->getifun()->setInput(ifun);
    ereg->getvalC()->setInput(valC);
    ereg->getvalA()->setInput(valA);
    ereg->getvalB()->setInput(valB);
    ereg->getdstE()->setInput(dstE);
    ereg->getdstM()->setInput(dstM);
    ereg->getsrcA()->setInput(srcA);
    ereg->getsrcB()->setInput(srcB);
}

/*
 * setSrcA
 * method that takes the icode and rA from the register pipeline and sets srcA.
 *
 * @param: icode - value from icode pipeline register within D
 * @param: D_rA - value from rA pipeline register within D
 *
 * @return: uint64_t - srcA value to be stored in the srcA pipeline register within E.
 */
uint64_t DecodeStage::setSrcA(uint64_t icode, uint64_t D_rA)
{
    uint64_t d_srcA;

    switch(icode)
    {
        case(IRRMOVQ):
        case(IRMMOVQ):
        case(IOPQ):
        case(IPUSHQ):
            d_srcA = D_rA;
            break;
        case(IPOPQ):
        case(IRET):
            d_srcA= RSP;
            break;
        default:
            d_srcA = RNONE;
    }

    return d_srcA;
}

/*
 * setSrcB
 * method that takes the icode and rB from the register pipeline and sets srcB.
 *
 * @param: icode - value from icode pipeline register within D
 * @param: D_rB - value from rB pipeline register within D
 *
 * @return: uint64_t - srcB value to be stored in the srcB pipeline register within E.
 */
uint64_t DecodeStage::setSrcB(uint64_t icode, uint64_t D_rB)
{
    uint64_t d_srcB;

    switch(icode)
    {

        case(IOPQ):
        case(IRMMOVQ):
        case(IMRMOVQ):
            d_srcB = D_rB;
            break;
        case(IPUSHQ):
        case(IPOPQ):
        case(ICALL):
        case(IRET):
            d_srcB = RSP;
            break;
        default:
            d_srcB = RNONE; 
    }

    return d_srcB;
}

/*
 * setDstE
 * method that takes the icode and rB from the register pipeline and sets dstE.
 *
 * @param: icode - value from icode pipeline register within D
 * @param: D_rB - value from rB pipeline register within D
 *
 * @return: uint64_t - dstE value to be stored in the dstE pipeline register within E.
 */
uint64_t DecodeStage::setDstE(uint64_t icode, uint64_t D_rB)
{
    uint64_t d_dstE;

    switch(icode)
    {
        case(IRRMOVQ):
        case(IIRMOVQ):
        case(IOPQ):
            d_dstE = D_rB;
            break;
        case(IPUSHQ):
        case(IPOPQ):
        case(ICALL):
        case(IRET):
            d_dstE = RSP;
            break;
        default:
            d_dstE = RNONE;
    }

    return d_dstE;
}

/*
 * setDstM
 * method that takes the icode and rA from the register pipeline and sets dstM.
 *
 * @param: icode - value from icode pipeline register within D
 * @param: D_rA - value from rA pipeline register within D
 *
 * @return: uint64_t - dstM value to be stored in the dstM pipeline register within E.
 */
uint64_t DecodeStage::setDstM(uint64_t icode, uint64_t D_rA)
{
    uint64_t d_dstM;

    switch(icode)
    {
        case(IMRMOVQ):
        case(IPOPQ):
            d_dstM = D_rA;
            break;
        default:
            d_dstM = RNONE;
    }

    return d_dstM;
}

/*
 * setValA
 * method that takes the valA and srcA within D, dstE and valE from ExecuteStage, and
 * Memory register and Writeback register to set valA.
 *
 * @param: d_rvalA - value from valA pipeline register within D.
 * @param: d_srcA - value from srcA pipeline register within D.
 * @param: e_dstE - value from dstE pipeline register within ExecuteStage.
 * @param: e_valE - value from valE pipeline register within ExecuteStage.
 * @param: icode - value from icode pipeline register within D.
 * @param: D_valP - value from valP found in DecodeStage.
 * @param: m_valM - value obtained from Memory by MemoryStage.
 * @param: mreg - pointer to the M register instance.
 * @param: wreg - pointer to the W register instance.
 *
 * @return: uint64_t - value to be stored in the valA pipeline register within E.
 */
uint64_t DecodeStage::setValA(uint64_t d_rvalA, uint64_t d_srcA, uint64_t e_dstE, uint64_t e_valE, 
    uint64_t icode, uint64_t D_valP, uint64_t m_valM, M * mreg, W * wreg)
{
    uint64_t d_valA;

    uint64_t M_dstM = mreg->getdstM()->getOutput();
    uint64_t M_dstE = mreg->getdstE()->getOutput();
    uint64_t M_valE = mreg->getvalE()->getOutput();
    uint64_t W_dstM = wreg->getdstM()->getOutput();
    uint64_t W_valM = wreg->getvalM()->getOutput();
    uint64_t W_dstE = wreg->getdstE()->getOutput();
    uint64_t W_valE = wreg->getvalE()->getOutput();

    if (icode == ICALL || icode == IJXX)
    {
        d_valA = D_valP;
    }
    else if(d_srcA == RNONE)
    {
        d_valA = 0;
    }
    else if (d_srcA == e_dstE)
    {
        d_valA = e_valE;
    }
    else if (d_srcA == M_dstM)
    {
        d_valA = m_valM;
    }
    else if (d_srcA == M_dstE)
    {
        d_valA = M_valE;
    }
    else if (d_srcA == W_dstM)
    {
        d_valA = W_valM;
    }
    else if (d_srcA == W_dstE)
    {
        d_valA = W_valE;
    }
    else
    {
        d_valA = d_rvalA;
    }

    return d_valA;
}

/*
 * setValB
 * method that takes the valB and srcB within D, dstE and valE from ExecuteStage, and
 * Memory register and Writeback register to set valB.
 *
 * @param: d_rvalB - value from valB pipeline register within D.
 * @param: d_srcB - value from srcB pipeline register within D.
 * @param: e_dstE - value from dstE pipeline register within ExecuteStage.
 * @param: e_valE - value from valE pipeline register within ExecuteStage.
 * @param: m_valM - value obtained from Memory by MemoryStage.
 * @param: mreg - pointer to the M register instance.
 * @param: wreg - pointer to the W register instance.
 *
 * @return: uint64_t - value to be stored in the valB pipeline register within E.
 */
uint64_t DecodeStage::setValB(uint64_t d_rvalB, uint64_t d_srcB, uint64_t e_dstE, uint64_t e_valE, 
    uint64_t m_valM, M * mreg, W * wreg)
{
    uint64_t d_valB;

    uint64_t M_dstM = mreg->getdstM()->getOutput();
    uint64_t M_dstE = mreg->getdstE()->getOutput();
    uint64_t M_valE = mreg->getvalE()->getOutput();
    uint64_t W_dstM = wreg->getdstM()->getOutput();
    uint64_t W_valM = wreg->getvalM()->getOutput();
    uint64_t W_dstE = wreg->getdstE()->getOutput();
    uint64_t W_valE = wreg->getvalE()->getOutput();

    if (d_srcB == RNONE)
    {
        d_valB = 0;
    }
    else if (d_srcB == e_dstE)
    {
        d_valB = e_valE;
    }
    else if (d_srcB == M_dstM)
    {
        d_valB = m_valM;
    }
    else if (d_srcB == M_dstE)
    {
        d_valB = M_valE;
    }
    else if (d_srcB == W_dstM)
    {
        d_valB = W_valM;
    }
    else if (d_srcB == W_dstE)
    {
        d_valB = W_valE;
    }
    else
    {
        d_valB = d_rvalB;
    }

    return d_valB;
}

/*
 * getsrcA
 * Method to get the srcA
 *
 * @return d_srcA
 */
uint64_t DecodeStage::getsrcA()
{
    return srcA;
}

/*
 * getsrcB
 * Method to get the srcB
 *
 * @return d_srcB
 */
uint64_t DecodeStage::getsrcB()
{
    return srcB;
}

/*
 * calculateControlSignals
 * Method to calculate the control signal of the D pipeline register.
 *
 * @param ereg - E stage pipeline registers
 * @param stages - Stages of registers
 *
 * @return true if E stage needs to be bubbled, false otherwise.
 */
bool DecodeStage::calculateControlSignals(E * ereg, Stage ** stages)
{
    uint64_t E_icode = ereg->geticode()->getOutput();
    uint64_t E_dstM = ereg->getdstM()->getOutput(); 

    Stage * exe = stages[EREG];
    uint64_t e_Cnd = ((ExecuteStage *)exe)->getCnd();

    if ((E_icode == IJXX && !e_Cnd) || ((E_icode == IMRMOVQ || E_icode == IPOPQ) && (E_dstM == srcA || E_dstM == srcB)))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/*
 * bubbleE
 * Method that bubbles E if E_bubble is true.
 *
 * @param ereg - E stage pipeline register
 */
void DecodeStage::bubbleE(E * ereg)
{
    ereg->getstat()->bubble(SAOK);
    ereg->geticode()->bubble(INOP);
    ereg->getifun()->bubble();
    ereg->getvalC()->bubble();
    ereg->getvalA()->bubble();
    ereg->getvalB()->bubble();
    ereg->getdstE()->bubble(RNONE);
    ereg->getdstM()->bubble(RNONE);
    ereg->getsrcA()->bubble(RNONE);
    ereg->getsrcB()->bubble(RNONE);
}

/*
 * normalE
 * Method that send D stage registers into E stage registers.
 *
 * @param ereg - E stage pipeline register.
 */
void DecodeStage::normalE(E * ereg)
{
    ereg->getstat()->normal();
    ereg->geticode()->normal();
    ereg->getifun()->normal();
    ereg->getvalC()->normal();
    ereg->getvalA()->normal();
    ereg->getvalB()->normal();
    ereg->getdstE()->normal();
    ereg->getdstM()->normal();
    ereg->getsrcA()->normal();
    ereg->getsrcB()->normal();
}

