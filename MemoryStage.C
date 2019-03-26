#include <string>
#include <cstdint>
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
#include "MemoryStage.h"
#include "Instructions.h"
#include "Memory.h"

/*
 * doClockLow:
 * Performs the Memory stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool MemoryStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    M * mreg = (M *) pregs[MREG];
    W * wreg = (W *) pregs[WREG];
    Memory * mem = Memory::getInstance();
   
    uint64_t icode = mreg->geticode()->getOutput();
    uint64_t valE = mreg->getvalE()->getOutput();
    uint64_t dstE = mreg->getdstE()->getOutput();
    uint64_t dstM = mreg->getdstM()->getOutput();
    uint64_t valA = mreg->getvalA()->getOutput();
    
    uint64_t addr = addr_Component(icode, valE, valA);
    bool error = false;

    if(memRead_Component(icode))
    {
        m_valM = mem->getLong(addr, error);
    }
    else
    {
        m_valM = 0;
    }

    if(memWrite_Component(icode))
    {
        mem->putLong(valA, addr, error);
    }
    
    if (error)
    {
        m_stat = SADR;
    }
    else
    {
        m_stat = mreg->getstat()->getOutput();
    }

    setWInput(wreg, m_stat, icode, valE, m_valM, dstE, dstM);

    return false;
}

/* doClockHigh
 * applies the appropriate control signal to the -- register intances
 *
 * @param: pregs - array of the pipeline register (F, D, E, M, W instances)
 */
void MemoryStage::doClockHigh(PipeReg ** pregs)
{ 
    W * wreg = (W *) pregs[WREG];

    wreg->getstat()->normal();
    wreg->geticode()->normal();
    wreg->getvalE()->normal();
    wreg->getvalM()->normal();
    wreg->getdstE()->normal();
    wreg->getdstM()->normal();
}

/**
 * setWInput
 *
 * provides the input to potentially be stored in W register during
 * doClockHigh
 *
 * @param: wreg - pointer to the W register instance
 * @param: stat - value to be stored in the stat pipeline register within W
 * @param: icode - value to be stored in the icode pipeline register within W
 * @param: valE - value to be stored in the rA pipeline register within W
 * @param: valA - value to be stored in the rB pipeline register within W
 * @param: dstE - value to be stored in the dstE pipeline register within W
 * @param: dstM - value to be stored in the dstM pipeline register within W
 */
void MemoryStage::setWInput(W * wreg, uint64_t stat, uint64_t icode, uint64_t valE, uint64_t valM, uint64_t dstE, uint64_t dstM)
{
    wreg->getstat()->setInput(stat);
    wreg->geticode()->setInput(icode);
    wreg->getvalE()->setInput(valE);
    wreg->getvalM()->setInput(valM);    
    wreg->getdstE()->setInput(dstE);
    wreg->getdstM()->setInput(dstM);
}

/*
 * addr_Component
 * Method that calculates memory address.
 *
 * @param M_icode - value of icode from Memory register.
 * @param M_valE - value of valE from Memory register.
 * @param M_valA - value of valA from Memory register.
 *
 * @return mem_addr.
 */
uint64_t MemoryStage::addr_Component(uint64_t M_icode, uint64_t M_valE, uint64_t M_valA)
{
    uint64_t mem_addr;
    
    switch(M_icode)
    {
        case(IRMMOVQ):
        case(IPUSHQ):
        case(ICALL):
        case(IMRMOVQ):
            mem_addr = M_valE;
            break;
        case(IPOPQ):
        case(IRET):
            mem_addr = M_valA;
            break;
        default:
            mem_addr = 0;
    }
    
    return mem_addr;
}

/*
 * memRead_component
 * Method that determines if memory needs to be read.
 *
 * @param icode - value of icode from Memory register.
 *
 * @return true if memory needs to be read, false otherwise.
 */
bool MemoryStage::memRead_Component(uint64_t M_icode)
{
    switch(M_icode)
    {
        case(IMRMOVQ):
        case(IPOPQ):
        case(IRET):
            return true;
        default:
            return false;
    }
}

/*
 * memWrite_Component
 * Method that determines if Memory needs to be written to.
 *
 * @param icode - value of icode from Memory Stage.
 *
 * @return true if Memory needs to be written to, false otherwise.
 */
bool MemoryStage::memWrite_Component(uint64_t M_icode)
{
    switch(M_icode)
    {
        case(IRMMOVQ):
        case(IPUSHQ):
        case(ICALL):
            return true;
        default:
            return false;
    }
}

/*
 * getm_valM
 * Method that returns valM from the Memory Stage.
 *
 * @return m_valM
 */
uint64_t MemoryStage::getm_valM()
{
    return m_valM;
}

/*
 * getm_stat
 * Method that returns stat from Memory Stage.
 *
 * @return m_stat
 */
uint64_t MemoryStage::getm_stat()
{
    return m_stat;
}

