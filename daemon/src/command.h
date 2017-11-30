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
                   AUTO };

    Command( const char* buffer, uint32_t size );
    ~Command();

    CmdType command() const;

private:
    CmdType _command;
};

#endif // COMMAND_H
