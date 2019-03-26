//class to perform the combinational logic of
//the Fetch stage
class FetchStage: public Stage
{
    private:
        bool F_stall;
        bool D_stall;
        bool D_bubble;
        void setDInput(D * dreg, uint64_t stat, uint64_t icode, uint64_t ifun, 
            uint64_t rA, uint64_t rB, uint64_t valC, uint64_t valP);
        uint64_t selectPC(F * freg, M * mreg, W * wreg);
        bool needRegIds(uint64_t icode, uint64_t f_pc, bool & error, uint64_t & rA, uint64_t & rB);
        bool needValC(uint64_t icode, uint64_t & valC, uint64_t f_pc, bool & error);
        uint64_t predictPC(uint64_t icode, uint64_t valC, uint64_t valP);
        uint64_t PCIncrement(uint64_t f_pc, uint64_t icode, bool needRegID, bool needValC);
        uint64_t setf_stat(uint64_t icode, bool & error);
        bool instr_valid(uint64_t icode);
        bool setF_stall(uint64_t D_icode, uint64_t E_icode, uint64_t M_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB);
        bool setD_stall(uint64_t E_icode, uint64_t E_dstM, uint64_t d_srcA, uint64_t d_srcB);
        void calculateControlSignals(D * dreg, E * ereg, M * mreg, Stage ** stages);
        bool setD_bubble(uint64_t D_icode, uint64_t E_icode, uint64_t M_icode, uint64_t e_Cnd, uint64_t E_dstM,
            uint64_t d_srcA, uint64_t d_srcB);
        void bubbleD(D * dreg);
        void normalD(D * dreg);
    public:
        bool doClockLow(PipeReg ** pregs, Stage ** stages);
        void doClockHigh(PipeReg ** pregs);
};
