CC = g++
CFLAGS = -g -c -Wall -std=c++11 -Og
OBJ = yess.o Memory.o Tools.o RegisterFile.o \
ConditionCodes.o Loader.o FetchStage.o DecodeStage.o ExecuteStage.o \
MemoryStage.o WritebackStage.o PipeReg.o PipeRegField.o Simulate.o F.o \
D.o E.o M.o W.o

STGS = RegisterFile.h PipeRegField.h PipeReg.h F.h D.h E.h M.h W.h Stage.h Status.h Debug.h
ST = PipeRegField.h PipeReg.h F.h D.h E.h M.h W.h Stage.h ExecuteStage.h MemoryStage.h \
DecodeStage.h FetchStage.h WritebackStage.h Simulate.h Memory.h RegisterFile.h ConditionCodes.h
SING = Instructions.h RegisterFile.h PipeReg.h PipeRegField.h

.C.o:
	$(CC) $(CFLAGS) $< -o $@

yess: $(OBJ)

yess.o: Debug.h Memory.h Loader.h RegisterFile.h ConditionCodes.h PipeReg.h Stage.h Simulate.h

Memory.o: Memory.h Tools.h

Tools.o: Tools.h

RegisterFile.o: RegisterFile.h Tools.h

ConditionCodes.o: ConditionCodes.h Tools.h

Loader.o: Loader.h Memory.h

FetchStage.o: $(STGS) Instructions.h Memory.h Tools.h FetchStage.h DecodeStage.h ExecuteStage.h

DecodeStage.o: $(STGS) Instructions.h DecodeStage.h ExecuteStage.h MemoryStage.h

ExecuteStage.o: $(STGS) Instructions.h ExecuteStage.h MemoryStage.h ConditionCodes.h Tools.h

MemoryStage.o: $(STGS) MemoryStage.h Instructions.h Memory.h

WritebackStage.o: $(STGS) WritebackStage.h Instructions.h

PipeReg.o: PipeReg.h

PipeRegField.o: PipeRegField.h

Simulate.o: $(ST)

F.o: PipeRegField.h PipeReg.h F.h

D.o: $(SING) D.h Status.h

E.o: RegisterFile.h Instructions.h PipeRegField.h PipeReg.h E.h Status.h

M.o: RegisterFile.h Instructions.h PipeRegField.h PipeReg.h M.h Status.h

W.o: RegisterFile.h Instructions.h PipeRegField.h PipeReg.h W.h Status.h

clean:
	rm $(OBJ) yess

run:
	make clean
	make yess
	./run.sh

