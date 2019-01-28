#include "command.h"

#include <stdio.h>

std::vector<std::set< std::pair<std::string, Command::CmdType> > > Command::_commands;
std::size_t Command::_commands_size = 0;

void Command::init()
{
    _commands.resize(19);
    _commands[4].insert( std::pair<std::string, CmdType>("auto", AUTO) );
    _commands[6].insert( std::pair<std::string, CmdType>("gas-on", GAS_ON) );
    _commands[6].insert( std::pair<std::string, CmdType>("manual", MANUAL) );
    _commands[7].insert( std::pair<std::string, CmdType>("program", PROGRAM) );
    _commands[7].insert( std::pair<std::string, CmdType>("gas-off", GAS_OFF) );
    _commands[8].insert( std::pair<std::string, CmdType>("max-temp", SET_MAX_TEMP) );
    _commands[8].insert( std::pair<std::string, CmdType>("min-temp", SET_MIN_TEMP) );
    _commands[8].insert( std::pair<std::string, CmdType>("ext-temp", EXT_TEMP) );
    _commands[8].insert( std::pair<std::string, CmdType>("activate", ACTIVATE) );
    _commands[9].insert( std::pair<std::string, CmdType>("pellet-on", PELLET_ON) );
    _commands[10].insert( std::pair<std::string, CmdType>("pellet-off", PELLET_OFF) );
    _commands[10].insert( std::pair<std::string, CmdType>("deactivate", DEACTIVATE) );
    _commands[12].insert( std::pair<std::string, CmdType>("template-set", TEMPLATE_SET) );
    _commands[12].insert( std::pair<std::string, CmdType>("set-hyst-max", SET_HISTERESYS_MAX) );
    _commands[12].insert( std::pair<std::string, CmdType>("set-hyst-min", SET_HISTERESYS_MIN) );
    _commands[13].insert( std::pair<std::string, CmdType>("smart-temp-on", SMART_TEMP_ON) );
    _commands[14].insert( std::pair<std::string, CmdType>("smart-temp-off", SMART_TEMP_OFF) );
    _commands[17].insert( std::pair<std::string, CmdType>("pellet-minimum-on", PELLET_MINIMUM_ON) );
    _commands[18].insert( std::pair<std::string, CmdType>("pellet-minimum-off", PELLET_MINIMUM_OFF) );
    _commands_size = _commands.size();
}

Command::Command(const char *buffer, uint32_t size):
    _command(INVALID),
    _param(),
    _cmd_string( buffer, 4 )
{    
    bool found = false;
    // Non sprecare cicli, non controllare oltre la massima lunghezza del comando:
    std::size_t check_size = (size+1) > _commands_size ? _commands_size : size+1;
    for ( uint32_t i = 4;
          (i < check_size) && !found; i++ )
    {
        for ( std::set<std::pair<std::string, CmdType> >::const_iterator x = _commands[i].begin();
              (x != _commands[i].end()) &&
              !found; ++x )
        {
            if ( _cmd_string == x->first )
            {
                found = true;
                _command = x->second;
                _param = std::string( &buffer[i], size-i );
            }
        }
        if ( !found )
            _cmd_string += buffer[i];
    }
}

Command::~Command()
{

}

Command::CmdType Command::command() const
{
    return _command;
}

std::string Command::commandStr() const
{
    return _cmd_string;
}

std::string Command::getParam() const
{
    return _param;
}
