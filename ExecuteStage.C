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
#include "ExecuteStage.h"
#include "MemoryStage.h"
#include "ConditionCodes.h"
#include "Tools.h"

/*
 * doClockLow:
 * Performs the Execute stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool ExecuteStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{    
    E * ereg = (E *) pregs[EREG];
    M * mreg = (M *) pregs[MREG];
    W * wreg = (W *) pregs[WREG];

    uint64_t stat = ereg->getstat()->getOutput();
    uint64_t icode = ereg->geticode()->getOutput();
    uint64_t ifun = ereg->getifun()->getOutput();
    uint64_t valC = ereg->getvalC()->getOutput();
    uint64_t valA = ereg->getvalA()->getOutput();
    uint64_t valB = ereg->getvalB()->getOutput();
    uint64_t dstM = ereg->getdstM()->getOutput();

    uint64_t aluA = setaluA(icode, valA, valC);
    uint64_t aluB = setaluB(icode, valB);
    uint64_t alufun = setalufun(icode, ifun);
    
    e_valE = doALU(aluA, aluB, alufun);
    
    if (setcc(icode, stages, wreg))
    {
        setCC_component(aluA, aluB, alufun, e_valE);
    }
    
    int64_t W_stat = wreg->getstat()->getOutput();

    Stage * mem = stages[MREG];
    uint64_t m_stat = ((MemoryStage *)mem)->getm_stat();
  
    Cnd = cond(icode, ifun);

    e_dstE = setdstE(icode, Cnd, ereg->getdstE()->getOutput());

    M_bubble = calculateControlSignals(m_stat, W_stat);

    setMInput(mreg, stat, icode, Cnd, e_valE, valA, e_dstE, dstM);

    return false;
}

/*
 * doClockHigh
 * applies the appropriate control signal to the M register instance.
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void ExecuteStage::doClockHigh(PipeReg ** pregs)
{   
    M * mreg = (M *) pregs[MREG];
    
    if (M_bubble)
    {
        mreg->getstat()->bubble(SAOK);
        mreg->geticode()->bubble(INOP);
        mreg->getCnd()->bubble();
        mreg->getvalE()->bubble();
        mreg->getvalA()->bubble();
        mreg->getdstE()->bubble(RNONE);
        mreg->getdstM()->bubble(RNONE);
    }
    else
    {
        mreg->getstat()->normal();
        mreg->geticode()->normal();
        mreg->getCnd()->normal();
        mreg->getvalE()->normal();
        mreg->getvalA()->normal();
        mreg->getdstE()->normal();
        mreg->getdstM()->normal();
    }
}

/*
 * setMInput
 * provides the input to be stored in the M register during doClockHigh.
 *
 * @param: mreg - pointer to the M register instance.
 * @param: stat - value to be stored in the stat pipeline register within M.
 * @param: icode - value to be stored in the icode pipeline register within M.
 * @param: Cnd - value to be stored in the Cnd pipeline register within M.
 * @param: valE - value to be stored in the valE pipeline register within M.
 * @param: valA - value to be stored in the valA pipeline register within M.
 * @param: dstE - value to be stored in the dstE pipeline register within M.
 * @param: dstM - value to be stored in the dstM pipeline register within M.
 */
void ExecuteStage::setMInput(M * mreg, uint64_t stat, uint64_t icode,
        uint64_t Cnd, uint64_t valE, uint64_t valA, uint64_t dstE, uint64_t dstM) 
{
    mreg->getstat()->setInput(stat);
    mreg->geticode()->setInput(icode);
    mreg->getCnd()->setInput(Cnd);
    mreg->getvalE()->setInput(valE);
    mreg->getvalA()->setInput(valA);
    mreg->getdstE()->setInput(dstE);
    mreg->getdstM()->setInput(dstM);
}

/*
 * setaluA
 * provides the input to be used in setting the cc_component
 *
 * @param: icode - value from icode pipeline register within E.
 * @param: valA - value from valA pipeline register within E.
 * @param: valC - value from valC pipeline register within E.
 *
 * @return: uint64_t - aluA value to be used in setting cc_component. 
 */
uint64_t ExecuteStage::setaluA(uint64_t E_icode, uint64_t E_valA, uint64_t E_valC)
{
    uint64_t aluA;

    switch(E_icode)
    {
        case(IRRMOVQ):
        case(IOPQ):
            aluA = E_valA;
            break;
        case(IIRMOVQ):
        case(IRMMOVQ):
        case(IMRMOVQ):
            aluA = E_valC;
            break;
        case(ICALL):
        case(IPUSHQ):
            aluA = -8;
            break;
        case(IRET):
        case(IPOPQ):
            aluA = 8;
            break;
        default:
            aluA = 0;
    }
    return aluA;
}

/*
 * setaluB
 * provides the input to be used in setting the cc_component
 *
 * @param: icode - value from icode pipeline register within E.
 * @param: valB - value from valB pipeline register within E.
 *
 * @return: uint64_t - aluB value to be used in setting cc_component. 
 */
uint64_t ExecuteStage::setaluB(uint64_t E_icode, uint64_t E_valB)
{
    uint64_t aluB;

    switch(E_icode)
    {
        case(IRMMOVQ):
        case(IMRMOVQ):
        case(IOPQ):
        case(ICALL):
        case(IPUSHQ):
        case(IRET):
        case(IPOPQ):
            aluB = E_valB;
            break;

        case(IRRMOVQ):
        case(IIRMOVQ):
            aluB = 0;
            break;

        default:
            aluB = 0;
    }
    return aluB;
}

/*
 * setalufun
 * method that is to be used in setting the cc_component
 *
 * @param: icode - value from icode pipeline register within E.
 * @param: ifun - value from ifun pipeline register within E.
 *
 * @return: uint64_t - alufun value to be used in setting cc_component. 
 */
uint64_t ExecuteStage::setalufun(uint64_t E_icode, uint64_t E_ifun)
{
    uint64_t alufun;

    if(E_icode == IOPQ)
    {   
        alufun = E_ifun;
    }
    else
    {
        alufun = ADDQ;
    }

    return alufun;
}

/*
 * setcc
 * true or false depending on icode.
 *
 * @param: icode - value from icode pipeline register within E.
 */
bool ExecuteStage::setcc(uint64_t E_icode, Stage ** stages, W * wreg)
{
    uint64_t W_stat = wreg->getstat()->getOutput();

    Stage * mem = stages[MREG];
    uint64_t m_stat = ((MemoryStage *)mem)->getm_stat();
    
    return((E_icode == IOPQ) && !ismStat(m_stat) && !iswStat(W_stat));
}

/*
 * ismstat
 * true or false depending on is the stat from Memory Stage is needed.
 *
 * @param: m_stat - stat from Mem Stage.
 */
bool ExecuteStage::ismStat(uint64_t m_stat)
{
    switch(m_stat)
    {
        case(SADR):
        case(SINS):
        case(SHLT):
            return true;
            break;
        default:
            return false;
    }
}

/*
 * iswstat
 * true or false depending on if the stat from Writeback Stage is needed.
 *
 * @param: W_stat - state from Writeback Stage.
 */
bool ExecuteStage::iswStat(uint64_t W_stat)
{
    switch(W_stat)
    {
        case(SADR):
        case(SINS):
        case(SHLT):
            return true;
            break;
        default:
            return false;
    }
}

/*
 * setdstE
 * method that sets dstE in pipeline register within E.
 *
 * @param: icode - value from icode pipeline register within E.
 * @param: Cnd - value from Cnd pipeline register within E.
 * @param: dstE - value from dstE pipeline register from ereg pipeline.
 */
uint64_t ExecuteStage::setdstE(uint64_t E_icode, uint64_t e_Cnd, uint64_t E_dstE)
{
    uint64_t e_dstE;

    if(E_icode == IRRMOVQ && !e_Cnd)
    {
        e_dstE = RNONE;
    }
    else
    {
        e_dstE = E_dstE;
    }
    return e_dstE;
}

/*
 * setCC_component
 * method that sets condition codes based off of alu in ExecuteStage.
 *
 * @param: aluA - value from aluA pipeline register.
 * @param: aluB - value from aluB pipeline register.
 * @param: alufun - value from alufun pipeline register within ExecuteStage.
 * @param: aluoutput - value from aluoutput pipeline register within ExecuteStage.
 */
void ExecuteStage::setCC_component(uint64_t aluA, uint64_t aluB, uint64_t alufun, uint64_t aluoutput)
{
    ConditionCodes * cd = ConditionCodes::getInstance();
    bool error = false;
    bool addqTF = Tools::addOverflow(aluA, aluB);
    bool subqTF = Tools::subOverflow(aluA, aluB);

    switch(alufun)
    {
        case(ADDQ):
            cd->setConditionCode(addqTF, OF, error);
            cd->setConditionCode((aluoutput == 0), ZF, error);
            cd->setConditionCode((Tools::sign(aluoutput) != 0), SF, error);
            break;
        case(SUBQ):
            cd->setConditionCode(subqTF, OF, error);
            cd->setConditionCode((aluoutput == 0), ZF, error);
            cd->setConditionCode((Tools::sign(aluoutput) != 0), SF, error);
            break;
        case(ANDQ):
            cd->setConditionCode(false, OF, error);
            cd->setConditionCode((aluoutput == 0), ZF, error);
            cd->setConditionCode((Tools::sign(aluoutput) != 0), SF, error);
            break;
        case(XORQ):
            cd->setConditionCode(false, OF, error);
            cd->setConditionCode((aluoutput == 0), ZF, error);
            cd->setConditionCode((Tools::sign(aluoutput) != 0), SF, error);
            break;            
    }
}

/*
 * doAlu
 * Method that uses aluA and aluB from ExecuteStage and alufun to choose alu function.
 *
 * @param: aluA - value from setaluA method.
 * @param: aluB - value from setaluB method.
 * @param: alufun - value from alufun method.
 *
 * @return: uint64_t - returns answer from the alu in ExecuteStage.
 */
uint64_t ExecuteStage::doALU(uint64_t aluA, uint64_t aluB, uint64_t alufun)
{
    switch(alufun)
    { 
        case(ADDQ):
            return (aluA + aluB);
        case(SUBQ):
            return (aluB - aluA);
        case(XORQ):
            return (aluA ^ aluB);
        case(ANDQ):
            return (aluA & aluB);
        default:
            return 0;
    }
}

/*
 * gete_dstE
 * method that returns the dstE from the ExecuteStage.
 *
 * @return: dstE
 */
uint64_t ExecuteStage::gete_dstE()
{
    return e_dstE;
}

/*
 * gete_valE
 * method that returns the valE from the ExecuteStage.
 *
 * @return: valE
 */
uint64_t ExecuteStage::gete_valE()
{
    return e_valE;
}

/*
 * cond
 * method that sets Cnd based on condition codes.
 *
 * @return: uint64_t - returns Cnd for M.
 */
uint64_t ExecuteStage::cond(uint64_t icode, uint64_t ifun)
{
    uint64_t e_Cnd;
    bool error = false;
    ConditionCodes * cc = ConditionCodes::getInstance();
    bool sf = cc->getConditionCode(SF, error);
    bool zf = cc->getConditionCode(ZF, error);
    bool of = cc->getConditionCode(OF, error);

    switch(icode){
        case(IJXX):
        case(ICMOVXX):
            switch(ifun) {
                case(UNCOND):
                    e_Cnd = 1;
                    break;
                case(LESSEQ):
                    e_Cnd = ((sf ^ of) | zf);
                    break;
                case(LESS):
                    e_Cnd = (sf ^ of);
                    break;
                case(EQUAL):
                    e_Cnd = zf;
                    break;
                case(NOTEQUAL):
                    e_Cnd = !zf;
                    break;
                case(GREATER):
                    e_Cnd = (!(sf ^ of)) & !zf;
                    break;
                case(GREATEREQ):
                    e_Cnd = (!(sf ^ of));
                    break;
                default:
                    e_Cnd = 0;
            }
            break;
        default:
            e_Cnd = 0;
    }

    return e_Cnd;
}

/*
 * calculateControlSignals
 * Method that calculates if M stage needs to be bubbled.
 *
 * @param m_stat - stat code from Memory Stage forward.
 * @param W_stat - stat code from Writeback Stage forward.
 *
 * @return true if M stage needs to be bubbled, false otherwise.
 */
bool ExecuteStage::calculateControlSignals(uint64_t m_stat, uint64_t W_stat)
{
    return (ismStat(m_stat) || iswStat(W_stat));
}

/*
 * getCnd
 * Method that gets the Cnd code from Execute Stage after it has been calculated.
 *
 * @return Cnd
 */
uint64_t ExecuteStage::getCnd()
{
    return Cnd;
}

