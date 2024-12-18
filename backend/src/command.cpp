#include "command.h"


Command::Command(uint32_t command_id, double param, const std::string& string )
    :_command( static_cast<CmdType>(command_id) )
    , _param( param )
    , _param_str( string )
{    
}

Command::~Command()
{
}

Command::CmdType Command::command() const
{
    return _command;
}

double Command::getParam() const
{
    return _param;
}

std::string Command::getParamString() const
{
    return _param_str;
}

