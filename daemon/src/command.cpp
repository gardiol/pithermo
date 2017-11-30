#include "command.h"

Command::Command(const char *buffer, uint32_t size):
    _command(INVALID)
{
    if ( (size == 10) && (std::string(buffer,10) == "pellet-off") )
        _command = PELLET_OFF;
    else if ( (size == 9) && (std::string(buffer,9) == "pellet-on") )
        _command = PELLET_ON;
    else if ( (size == 7) && (std::string(buffer,7) == "gas-off") )
        _command = GAS_OFF;
    else if ( (size == 6) && (std::string(buffer,6) == "manual") )
        _command = MANUAL;
    else if ( (size == 6) && (std::string(buffer,6) == "gas-on") )
        _command = GAS_ON;
    else if ( (size == 4) && (std::string(buffer,4) == "auto") )
        _command = AUTO;
}

Command::~Command()
{

}

Command::CmdType Command::command() const
{
    return _command;
}
