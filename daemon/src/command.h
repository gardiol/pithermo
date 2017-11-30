#ifndef COMMAND_H
#define COMMAND_H

#include "common_defs.h"
#include <string>

class Command
{
public:
    enum CmdType { INVALID,
                   PELLET_ON,
                   PELLET_OFF,
                   GAS_ON,
                   GAS_OFF,
                   MANUAL,
                   SET_MIN_TEMP,
                   SET_MAX_TEMP,
                   PROGRAM,
                   AUTO };

    Command( const char* buffer, uint32_t size );
    ~Command();

    CmdType command() const;
    std::string getParam() const;

private:
    CmdType _command;
    std::string _param;
};

#endif // COMMAND_H
