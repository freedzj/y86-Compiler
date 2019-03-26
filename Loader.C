/**
 * Names: Justyn Hanna, Zach Freed
 * Team: 13
 */
#include <iostream>
#include <fstream>
#include <string.h>
#include <ctype.h>
#include "Loader.h"
#include "Memory.h"

//first column in file is assumed to be 0
#define ADDRBEGIN 2   //starting column of 3 digit hex address 
#define ADDREND 4     //ending column of 3 digit hext address
#define DATABEGIN 7   //starting column of data bytes
#define COMMENT 28    //location of the '|' character 

/**
 * Loader constructor
 * Opens the .yo file named in the command line arguments, reads the contents of the file
 * line by line and loads the program into memory.  If no file is given or the file doesn't
 * exist or the file doesn't end with a .yo extension or the .yo file contains errors then
 * loaded is set to false.  Otherwise loaded is set to true.
 *
 * @param argc is the number of command line arguments passed to the main; should
 *        be 2
 * @param argv[0] is the name of the executable
 *        argv[1] is the name of the .yo file
 */
Loader::Loader(int argc, char * argv[])
{
    loaded = false;

    std::string inLine;
    int lineNumber = 0;
    int64_t lastAddr = -1;

    if (argc == 2)
    {
        if (openFile(argv))
        {
            while (std::getline(inf, inLine))
            {
                lineNumber++;
                if(hasError(inLine, lastAddr))
                {
                    std::cout << "Error on line " << std::dec << lineNumber
                        << ": " << inLine << std::endl;
                    return;
                }
                else if (hasData(inLine) && hasAddr(inLine))
                {
                    loadline(inLine, lastAddr);
                }
                else
                {
                    loaded = false;
                }
            }
            loaded = true;
        }
        else
        {
            loaded = false;
        }
    }   
    else
    {
        loaded = false;
    }
}

/**
 * isLoaded
 * returns the value of the loaded data member; loaded is set by the constructor
 *
 * @return value of loaded (true or false)
 */
bool Loader::isLoaded()
{
    return loaded;
}

/**
 * openFile()
 * Opens the file that takes as input the name, and returns true if the name of the file ends
 * with a .yo extension and if the file can be opened, otherwise returns false.
 *
 * @param name - name of file 
 *
 * @return boolean value if file is loaded or not.
 */
bool Loader::openFile(char * name[])
{
    int strSize = strlen(name[1]);
    std::string file = name[1];

    if (strSize >= 3)
    {
        if (file.compare((strSize - 3), strSize, ".yo") == 0)
        {
            inf.open(name[1], std::ifstream::in);

            if (!inf.is_open())
            {
                return false;
            }
            else
            {
                return true;
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

/**
 * loadline()
 * Method that takes a line from the .yo file that has an address and has data
 * and loads the data into memory at the indicated address.
 *
 * @param line - line from .yo file.
 */
void Loader::loadline(std::string line, int64_t & lastAddr)
{
    bool error = false;
    int loadByte = DATABEGIN;
    uint8_t byteData;

    uint32_t addr = convert(line, ADDRBEGIN, ADDREND);

    while (line.c_str()[loadByte] != ' ')
    {
        byteData = convert(line, loadByte, 2);
        Memory::getInstance()->putByte(byteData, addr, error);
        loadByte += 2;
        addr++;
    }

    lastAddr = addr;
}

/**
 * convert
 * Method that takes as input the line, starting, and ending columns of the
 * characters and returns the hex value.
 *
 * @param line - string from loadline
 * @param ADDRBEGIN - beginning column
 * @param ADDREND - ending column
 *
 * @return hex value
 */
uint32_t Loader::convert(std::string line, int sCol, int eCol)
{
    std::string in = line.substr(sCol, eCol);

    return std::stoul(in, NULL, 16);
}

/**
 * hasAddr
 * Method that checks to see if the line is an address.
 *
 * @param line - line from file
 *
 * @return bool
 */
bool Loader::hasAddr(std::string line)
{
    bool tf = false;

    if (line.c_str()[0] == '0')
    {
        tf = true;
    }
    else
    {
        tf = false;
    }

    return tf;
}

/**
 * getAddr
 * Method that gets the address of the line from input.
 *
 * @param line - line from file
 * @return address
 */
std::string Loader::getAddr(std::string line)
{
    if (hasAddr(line))
    {
        std::string addr = "";
        addr += line.c_str()[ADDRBEGIN];
        addr += line.c_str()[ADDRBEGIN + 1];
        addr += line.c_str()[ADDREND];

        return addr;
    }
    else
    {
        return NULL;
    }
}

/**
 * hasData
 * Method that checks to see if the line has data.
 *
 * @param line - line from file
 *
 * @return bool
 */
bool Loader::hasData(std::string line)
{
    bool tf = false;

    if (isblank(line.c_str()[DATABEGIN]))
    {
        tf = false;
    }
    else
    {
        tf = true;
    }

    return tf;
}

/**
 * getData
 * Method that gets the data from the line from input.
 *
 * @param line - line from input
 * @return data
 */
std::string Loader::getData(std::string line)
{
    int i = DATABEGIN;
    std::string data = "";
    while (!isblank(line.c_str()[i]))
    {
        data += line.c_str()[i];
        i++;
    }

    return data;
}

/**
 * hasError
 * Method that checks for errors in the address, data, comments, and spaces.
 *
 * @param line - line from file
 *
 * @return bool
 */
bool Loader::hasError(std::string line, int64_t lastAddr)
{
    bool tf = false;

    if (hasData(line) && hasAddr(line))
    {
        if (addrError(line, lastAddr) || dataError(line) || spaceError(line) || commentError(line) || colonError(line))
        {
            tf = true;
        }
        else
        {
            tf = false;
        }
    }
    else if (hasAddr(line))
    {
        int i = DATABEGIN;
        while (i < (COMMENT - 1))
        {
            if (line.c_str()[i] != ' ' || line.c_str()[i + 1] != ' ')
            {
                tf = true;
                break;
            }
            else
            {
                tf = false;
            }

            i += 2;
        }
    }
    else if (hasData(line) && !hasAddr(line))
    {
        tf = true;
    }
    else
    {
        if (commentError(line))
        {
            tf = true;
        }
        else
        {
            tf = false;
        }
    }

    return tf;
}

/**
 * addrError
 * Method that checks line from file and checks for any errors.
 *
 * @param line - line from file
 * @return bool
 */
bool Loader::addrError(std::string line, int64_t lastAddr)
{
    bool tf = false;

    if (line.c_str()[0] == '0' && line.c_str()[1] == 'x')
    {
        for (int i = ADDRBEGIN; i <= ADDREND; i++)
        {
            if (!isxdigit(line.c_str()[i]))
            {
                tf = true;
                break;
            }
        }

        std::string hexAddr = getAddr(line);
        std::string hexData = getData(line);
        int byteData = 0;
        for (int j = 0; j < 20; j += 2)
        {
            if (isxdigit(hexData.c_str()[j]) && isxdigit(hexData.c_str()[j + 1]))
            {
                byteData++;
            }
            else
            {
                break;
            }
        }

        int64_t addr = std::stoul(hexAddr, NULL, 16);
        int space = addr + byteData;

        if (space > MEMSIZE)
        {
            tf = true;
        }

        if (addr < lastAddr)
        {
            tf = true;
        }
    }
    else
    {
        tf = true;
    }

    return tf;
}

/**
 * dataError
 * Method that checks the data of the line.
 *
 * @param line - line from file
 * @return bool
 */
bool Loader::dataError(std::string line)
{
    bool tf = false;
    int i = DATABEGIN;

    if (!isxdigit(line.c_str()[DATABEGIN]) || !isxdigit(line.c_str()[DATABEGIN + 1]))
    {
        tf = true;
    }
    else if (byteNumber(line))
    {
        while (line.c_str()[i] != ' ') //new
        {
            if (!isxdigit(line.c_str()[i]) || !isxdigit(line.c_str()[i + 1]))
            {
                tf = true;
                break;
            }
            i += 2;
        }

        if (line.c_str()[i] == ' ')
        {
            for (int j = i; j < (COMMENT - 1); j++)
            {
                if (line.c_str()[j] != ' ')
                {
                    tf = true;
                    break;
                }
            }
        }
    }
    else
    {
        tf = true;
    }

    return tf;
}

/**
 * lineBlank
 * Method that checks to see if the line is blank.
 *
 * @param line - line from file
 * @return bool
 */
bool Loader::lineBlank(std::string line)
{
    if (line.c_str()[0] == ' ')
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * spaceError
 * Method that checks to see if the line has a space in the wrong place.
 *
 * @param line - line from file
 * @return bool
 */
bool Loader::spaceError(std::string line)
{
    if (line.c_str()[DATABEGIN - 1] != ' ' || line.c_str()[COMMENT - 1] != ' ' || line.c_str()[DATABEGIN] == ' ')
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * commentError
 * Method that checks to see if the line has a comment error.
 *
 * @param line - line from file
 * @return bool
 */
bool Loader::commentError(std::string line)
{
    if (line.c_str()[COMMENT] != '|')
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * colonError
 * Method that checks to see if the line has a colon error.
 *
 * @param line - line from file
 * @return bool
 */
bool Loader::colonError(std::string line)
{
    bool tf = false;

    if (line.c_str()[ADDREND + 1] != ':')
    {
        tf = true;
    }

    return tf;
}

/**
 * byteNumber
 * Method that finds how many bytes are in the data to make
 * sure that the data contains a multiple of 2.
 *
 * @param line - line from file.
 * @return bool - returns true if the file contains a multiple of 2.
 */
bool Loader::byteNumber(std::string line)
{
    bool tf = true;
    int i = DATABEGIN;

    std::string byteNum = "";
    if (!hasData(line))
    {
        return true;
    }

    while (line.c_str()[i] != ' ')
    {
        if (!isxdigit(line.c_str()[i]) || !isxdigit(line.c_str()[i + 1]))
        {
            return false;
        }
        else if (isxdigit(line.c_str()[i]) && isxdigit(line.c_str()[i + 1]))
        {
            return true;
        }
        i += 2;
    }

    return tf;
}

