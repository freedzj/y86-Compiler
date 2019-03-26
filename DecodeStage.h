class DecodeStage: public Stage
{
    private:
        uint64_t srcA;
        uint64_t srcB;
        bool E_bubble;
        void setEInput(E * ereg, uint64_t stat, uint64_t icode, uint64_t ifun, uint64_t valC, uint64_t valA, uint64_t valB, uint64_t dstE, uint64_t dstM, uint64_t srcA, uint64_t srcB);
        uint64_t setSrcA(uint64_t icode, uint64_t rA);
        uint64_t setSrcB(uint64_t icode, uint64_t rB);
        uint64_t setDstE(uint64_t icode, uint64_t rB);
        uint64_t setDstM(uint64_t icode, uint64_t rA);
        uint64_t setValA(uint64_t d_rvalA, uint64_t d_srcA, uint64_t e_dstE, uint64_t e_valE, 
            uint64_t icode, uint64_t D_valP, uint64_t m_valM, M * mreg, W * wreg);
        uint64_t setValB(uint64_t d_rvalB, uint64_t d_srcB, uint64_t e_dstE, uint64_t e_valE, 
            uint64_t m_valM, M * mreg, W * wreg);
        void bubbleE(E * ereg);
        void normalE(E * ereg);
        bool calculateControlSignals(E * ereg, Stage ** stages);
    public:
        bool doClockLow(PipeReg ** pregs, Stage ** stages);
        void doClockHigh(PipeReg ** pregs);
        uint64_t getsrcA();
        uint64_t getsrcB();
};

