#include <string>
#include <cstdint>
#include "Status.h"
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
#include "WritebackStage.h"
#include "Instructions.h"

/*
 * doClockLow:
 * Performs the Writeback stage combinational logic that is performed when
 * the clock edge is low.
 *
 * @param: pregs - array of the pipeline register sets (F, D, E, M, W instances)
 * @param: stages - array of stages (FetchStage, DecodeStage, ExecuteStage,
 *         MemoryStage, WritebackStage instances)
 */
bool WritebackStage::doClockLow(PipeReg ** pregs, Stage ** stages)
{
    W * wreg = (W *) pregs[WREG];
    bool tf = false;
    uint64_t stat = wreg->getstat()->getOutput();

    if (stat != SAOK)
    {
        tf = true;
    }
    else
    {
        tf = false;
    }
    return tf;
    
    RegisterFile * reg = RegisterFile::getInstance(); 

    uint64_t valE = wreg->getvalE()->getOutput();
    uint64_t dstE = wreg->getdstE()->getOutput();
    uint64_t valM = wreg->getvalM()->getOutput();
    uint64_t dstM = wreg->getdstM()->getOutput();
    bool error = false;

    reg->writeRegister(valE, (int32_t) dstE, error);
    reg->writeRegister(valM, (int32_t) dstM, error);
}

/*
 * doClockHigh
 * applies the appropriate control signal to the -- register instances
 *
 * @param: pregs - array of the pipleline register (F, D, E, M, W instances)
 */
void WritebackStage::doClockHigh(PipeReg ** pregs)
{
    W * wreg = (W *) pregs[WREG];
    RegisterFile * reg = RegisterFile::getInstance();

    uint64_t valE = wreg->getvalE()->getOutput();
    uint64_t dstE = wreg->getdstE()->getOutput();
    uint64_t valM = wreg->getvalM()->getOutput();
    uint64_t dstM = wreg->getdstM()->getOutput();
    bool error = false;

    reg->writeRegister(valE, (int32_t) dstE, error);
    reg->writeRegister(valM, (int32_t) dstM, error);
}

