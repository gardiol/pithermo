#include "program.h"

#include <stdio.h>

Program::Program()
{
    _gas_program.resize(7);
    _pellet_program.resize(7);
    for ( int d = 0; d < 7; d++ )
    {
        _gas_program[d].resize(24);
        _pellet_program[d].resize(24);
        for ( int h = 0; h < 24; h++ )
        {
            _gas_program[d][h].resize(2);
            _pellet_program[d][h].resize(2);
            for ( int f = 0; f < 2; f++ )
            {
                _gas_program[d][h][f] = false;
                _pellet_program[d][h][f] = false;
            }
        }
    }
}

Program::~Program()
{

}

bool Program::getPellet(int d, int h, int f)
{
    return _pellet_program[d][h][f];
}

bool Program::getGas(int d, int h, int f)
{
    return _gas_program[d][h][f];
}

bool Program::change(const std::string &p)
{
    bool ret = false;
    std::vector<std::string> days = FrameworkUtils::string_split( p, "]" );
    if ( days.size() >= 7 )
    {
        for ( int d = 0; d < 7; d++ )
        {
            std::string day_str = FrameworkUtils::string_replace(FrameworkUtils::string_replace(FrameworkUtils::string_replace(days[d], "[", "" ), "\"", ""), ",", "");
            for ( int h = 0; h < 24; h++ )
            {
                for ( int f = 0; f < 2; f++ )
                {
                    if ( day_str.length() > 0 )
                    {
                        char p = day_str[0];
                        day_str = day_str.substr( 1 );
                        bool gas_on = _gas_program[d][h][f];
                        bool pellet_on = _pellet_program[d][h][f];
                        if ( p == 'x' || p == 'X' )
                        {
                            if ( !ret && (!gas_on || !pellet_on) )
                                ret = true;
                            _gas_program[d][h][f] = true;
                            _pellet_program[d][h][f] = true;
                        }
                        else if ( p == 'g' || p == 'G' )
                        {
                            if ( !ret && (!gas_on || pellet_on) )
                                ret = true;
                            _gas_program[d][h][f] = true;
                            _pellet_program[d][h][f] = false;
                        }
                        else if ( p == 'p' || p == 'P' )
                        {
                            if ( !ret && (gas_on || !pellet_on) )
                                ret = true;
                            _gas_program[d][h][f] = false;
                            _pellet_program[d][h][f] = true;
                        }
                        else if ( p == 'o' || p == 'O' )
                        {
                            if ( !ret && (gas_on || pellet_on) )
                                ret = true;
                            _gas_program[d][h][f] = false;
                            _pellet_program[d][h][f] = false;
                        }
                    }
                }
            }
        }
    }
    return ret;
}

void Program::loadConfig(const ConfigData *c)
{
    if ( c != NULL )
    {
        for ( int d = 0; d < 7; d++ )
        {
            std::string day = "day"+FrameworkUtils::tostring(d);
            std::string day_string = c->getValue( day );
            std::vector<std::string> tokens = FrameworkUtils::string_split( day_string, ",");
            if ( tokens.size() >= 48 )
            {
                int t = 0;
                for ( int h = 0; h < 24; h++ )
                {
                    for ( int f = 0; f < 2; f++ )
                    {
                        std::string token = tokens[t++];
                        FrameworkUtils::string_tolower( token );
                        if ( (token == "g") || (token == "x") )
                            _gas_program[d][h][f] = true;
                        else
                            _gas_program[d][h][f] = false;
                        if ( (token == "p") || (token == "x") )
                            _pellet_program[d][h][f] = true;
                        else
                            _pellet_program[d][h][f] = false;
                    }
                }
            }
        }
    }
}

void Program::saveConfig(ConfigData *c)
{
    for ( int d = 0; d < 7; d++ )
    {
        std::string day_str = "day"+FrameworkUtils::tostring(d);
        std::string value = "";
        for ( int h = 0; h < 24; h++ )
        {
            for ( int f = 0; f < 2; f++ )
            {
                bool gas_on = _gas_program[d][h][f];
                bool pellet_on = _pellet_program[d][h][f];
                if ( gas_on && pellet_on )
                    value += "x";
                else if ( gas_on )
                    value += "g";
                else if ( pellet_on )
                    value += "p";
                else
                    value += "o";
                value += ",";
            }
        }
        c->setValue( day_str, value );
    }
}

void Program::writeJSON(FILE *file)
{
    char s[5] = "\"_\",";
    fwrite("[", 1, 1, file );
    for ( int d = 0; d < 7; d++ )
    {
        fwrite("[", 1, 1, file );
        for ( int h = 0; h < 24; h++ )
        {
            for ( int f = 0; f < 2; f++ )
            {
                bool pellet_on = _pellet_program[d][h][f];
                bool gas_on = _gas_program[d][h][f];
                s[1] = pellet_on ? (gas_on ? 'x' : 'p') : (gas_on ? 'g' : 'o');
                // disegna , solo alla fine della giornata:
                fwrite(s, ((h != 23) || (f != 1)) ? 4 : 3,1, file );
            }
        }
        // disegna , solo alla fine della lista:
        fwrite("],", (d != 6) ? 2 : 1, 1, file );
    }
    fwrite("]", 1, 1, file );
}
