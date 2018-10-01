#ifndef COMMAND_H
#define COMMAND_H

#include "common_defs.h"

#include <string>
#include <vector>
#include <set>

class Command
{
public:
    static void init();
    enum CmdType { INVALID,
                   ACTIVATE,
                   DEACTIVATE,
                   PELLET_ON,
                   PELLET_OFF,
                   PELLET_MINIMUM_ON,
                   PELLET_MINIMUM_OFF,
                   GAS_ON,
                   GAS_OFF,
                   MANUAL,
                   SET_MIN_TEMP,
                   SET_MAX_TEMP,
                   SET_HISTORY,
                   FLAMEOUT_RESET,
                   PROGRAM,
                   EXT_TEMP,
                   AUTO};

    Command( const char* buffer, uint32_t size );
    ~Command();

    CmdType command() const;
    std::string commandStr() const;
    std::string getParam() const;

private:
    CmdType _command;
    std::string _param;
    std::string _cmd_string;

    static std::vector<std::set< std::pair<std::string, CmdType> > > _commands;
    static std::size_t _commands_size;

};

#endif // COMMAND_H
