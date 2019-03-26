class ExecuteStage: public Stage
{
    private:
        uint64_t e_dstE;
        uint64_t e_valE;
        bool M_bubble;
        uint64_t Cnd;
        void setMInput(M * mreg, uint64_t stat, uint64_t icode, uint64_t Cnd, uint64_t valE, uint64_t valA,
            uint64_t dstE, uint64_t dstM);
        uint64_t setaluA(uint64_t E_icode, uint64_t E_valA, uint64_t E_valC);
        uint64_t setaluB(uint64_t E_icode, uint64_t E_valB);
        uint64_t setalufun(uint64_t E_icode, uint64_t E_ifun);
        bool setcc(uint64_t E_icode, Stage ** stages, W * wreg);
        uint64_t setdstE(uint64_t E_icode, uint64_t e_Cnd, uint64_t E_dstE);
        void setvalA(uint64_t icode, uint64_t & valA, uint64_t valC);
        void setCC_component(uint64_t aluA, uint64_t aluB, uint64_t alufun, uint64_t aluoutput);
        uint64_t doALU(uint64_t ifun, uint64_t aluA, uint64_t aluB);
        uint64_t cond(uint64_t icode, uint64_t ifun);
    	bool ismStat(uint64_t m_stat);
    	bool iswStat(uint64_t W_stat);
        bool calculateControlSignals(uint64_t m_stat, uint64_t W_stat);
    public:
        bool doClockLow(PipeReg ** pregs, Stage ** stages);
        void doClockHigh(PipeReg ** pregs);
        uint64_t gete_dstE();
        uint64_t gete_valE();
        uint64_t getCnd();
};
