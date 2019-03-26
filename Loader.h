class Loader
{
    private:
        bool openFile(char * name[]);
        void loadline(std::string line, int64_t & lastAddr);
        bool loaded;        //set to true if a file is successfully loaded into memory
        std::ifstream inf;  //input file handle
        uint32_t convert(std::string line, int sCol, int eCol);
        bool hasError(std::string line, int64_t lastAddr);
        bool addrError(std::string line, int64_t lastAddr);
        bool dataError(std::string line);
        bool spaceError(std::string line);
        bool commentError(std::string line);
        bool colonError(std::string line);
        bool hasAddr(std::string line);
        bool hasData(std::string line);
        bool lineBlank(std::string line);
        bool byteNumber(std::string line);
        std::string getAddr(std::string line);
        std::string getData(std::string line);
    public:
        Loader(int argc, char * argv[]);
        bool isLoaded();
};
