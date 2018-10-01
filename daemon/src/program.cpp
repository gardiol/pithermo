#include "program.h"

#include <stdio.h>

Program::Program():
    _d(0),
    _h(0),
    _f(0)
{
    _gas_program.resize(7);
    _pellet_program.resize(7);
    _pellet_minimum_program.resize(7);
    for ( std::size_t d = 0; d < 7; d++ )
    {
        _gas_program[d].resize(24);
        _pellet_program[d].resize(24);
        _pellet_minimum_program[d].resize(24);
        for ( std::size_t h = 0; h < 24; h++ )
        {
            _gas_program[d][h].resize(2);
            _pellet_program[d][h].resize(2);
            _pellet_minimum_program[d][h].resize(2);
            for ( std::size_t f = 0; f < 2; f++ )
            {
                _gas_program[d][h][f] = false;
                _pellet_program[d][h][f] = false;
                _pellet_minimum_program[d][h][f] = false;
            }
        }
    }
}

Program::~Program()
{

}

void Program::setTime(int d, int h, int f)
{
    if ( ( d >= 0 ) && ( d < 7 ) &&
         ( h >= 0 ) && ( h < 24 ) &&
         ( f >= 0 ) && ( f < 2 ) )
    {
        _d = static_cast<std::size_t>(d);
        _h = static_cast<std::size_t>(h);
        _f = static_cast<std::size_t>(f);
    }
}

bool Program::change(const std::string &p)
{
    bool ret = false;
    std::vector<std::string> days = FrameworkUtils::string_split( p, "]" );
    if ( days.size() >= 7 )
    {
        for ( std::size_t d = 0; d < 7; d++ )
        {
            std::string day_str = FrameworkUtils::string_replace(FrameworkUtils::string_replace(FrameworkUtils::string_replace(days[d], "[", "" ), "\"", ""), ",", "");
            for ( std::size_t h = 0; h < 24; h++ )
            {
                for ( std::size_t f = 0; f < 2; f++ )
                {
                    if ( day_str.length() > 0 )
                    {
                        char p = day_str[0];
                        day_str = day_str.substr( 1 );
                        bool gas_on = _gas_program[d][h][f];
                        bool pellet_on = _pellet_program[d][h][f];
                        bool pellet_minimum_on = _pellet_minimum_program[d][h][f];
                        if ( p == 'x' || p == 'X' )
                        {
                            if ( !ret && (!gas_on || !pellet_on) )
                                ret = true;
                            _gas_program[d][h][f] = true;
                            _pellet_program[d][h][f] = true;
                            _pellet_minimum_program[d][h][f] = false;
                        }
                        else if ( p == 'g' || p == 'G' )
                        {
                            if ( !ret && (!gas_on || pellet_on) )
                                ret = true;
                            _gas_program[d][h][f] = true;
                            _pellet_program[d][h][f] = false;
                            _pellet_minimum_program[d][h][f] = false;
                        }
                        else if ( p == 'p' || p == 'P' )
                        {
                            if ( !ret && (gas_on || !pellet_on) )
                                ret = true;
                            _gas_program[d][h][f] = false;
                            _pellet_program[d][h][f] = true;
                            _pellet_minimum_program[d][h][f] = false;
                        }
                        else if ( p == 'm' || p == 'M' )
                        {
                            if ( !ret && (gas_on || !pellet_minimum_on) )
                                ret = true;
                            _gas_program[d][h][f] = false;
                            _pellet_program[d][h][f] = true;
                            _pellet_minimum_program[d][h][f] = true;
                        }
                        else if ( p == 'o' || p == 'O' )
                        {
                            if ( !ret && (gas_on || pellet_on) )
                                ret = true;
                            _gas_program[d][h][f] = false;
                            _pellet_program[d][h][f] = false;
                            _pellet_minimum_program[d][h][f] = false;
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
    if ( c != nullptr )
    {
        for ( std::size_t d = 0; d < 7; d++ )
        {
            std::string day = "day"+FrameworkUtils::utostring(d);
            std::string day_string = c->getValue( day );
            std::vector<std::string> tokens = FrameworkUtils::string_split( day_string, ",");
            if ( tokens.size() >= 48 )
            {
                std::size_t t = 0;
                for ( std::size_t h = 0; h < 24; h++ )
                {
                    for ( std::size_t f = 0; f < 2; f++ )
                    {
                        std::string token = tokens[t++];
                        FrameworkUtils::string_tolower( token );
                        if ( (token == "g") || (token == "x") )
                            _gas_program[d][h][f] = true;
                        else
                            _gas_program[d][h][f] = false;
                        if ( (token == "p") || (token == "x") || (token == "m") )
                            _pellet_program[d][h][f] = true;
                        else
                            _pellet_program[d][h][f] = false;
                        if ( (token == "m") )
                            _pellet_minimum_program[d][h][f] = true;
                        else
                            _pellet_minimum_program[d][h][f] = false;
                    }
                }
            }
        }
    }
}

void Program::saveConfig(ConfigData *c)
{
    for ( std::size_t d = 0; d < 7; d++ )
    {
        std::string day_str = "day"+FrameworkUtils::utostring(d);
        std::string value = "";
        for ( std::size_t h = 0; h < 24; h++ )
        {
            for ( std::size_t f = 0; f < 2; f++ )
            {
                bool gas_on = _gas_program[d][h][f];
                bool pellet_on = _pellet_program[d][h][f];
                bool pellet_minimum_on = _pellet_minimum_program[d][h][f];
                if ( gas_on && pellet_on )
                    value += "x";
                else if ( gas_on )
                    value += "g";
                else if ( pellet_on )
                {
                    if ( pellet_minimum_on )
                        value += "m";
                    else
                        value += "p";
                }
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
    for ( std::size_t d = 0; d < 7; d++ )
    {
        fwrite("[", 1, 1, file );
        for ( std::size_t h = 0; h < 24; h++ )
        {
            for ( std::size_t f = 0; f < 2; f++ )
            {
                bool pellet_on = _pellet_program[d][h][f];
                bool gas_on = _gas_program[d][h][f];
                bool pellet_minimum_on = _pellet_minimum_program[d][h][f];
                s[1] = pellet_on ? (gas_on ? 'x' : (pellet_minimum_on ? 'm' : 'p') ) : (gas_on ? 'g' : 'o');
                // disegna , solo alla fine della giornata:
                fwrite(s, ((h != 23) || (f != 1)) ? 4 : 3,1, file );
            }
        }
        // disegna , solo alla fine della lista:
        fwrite("],", (d != 6) ? 2 : 1, 1, file );
    }
    fwrite("]", 1, 1, file );
}
