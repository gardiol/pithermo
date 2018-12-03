#include "program.h"

#include <stdio.h>

Program::Program():
    _d(0),
    _h(0),
    _f(0)
{
    _program.resize(7);
    for ( std::size_t d = 0; d < 7; d++ )
    {
        _program[d].resize(24);
        for ( std::size_t h = 0; h < 24; h++ )
        {
            _program[d][h].resize(2);
            for ( std::size_t f = 0; f < 2; f++ )
                _program[d][h][f] = LOW_GAS;
        }
    }
}

Program::~Program()
{

}

bool Program::useHigh() const
{
    ProgramType p = _program[_d][_h][_f];
    return p == Program::HIGH_AUTO ||
           p == Program::HIGH_GAS ||
           p == Program::HIGH_PELLET ;
}

bool Program::usePellet() const
{
    ProgramType p = _program[_d][_h][_f];
    return  p == Program::HIGH_PELLET ||
            p == Program::HIGH_AUTO ||
            p == Program::LOW_PELLET;
}

bool Program::useGas() const
{
    ProgramType p = _program[_d][_h][_f];
    return p == Program::HIGH_GAS ||
           p == Program::HIGH_AUTO ||
           p == Program::LOW_GAS;
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
    bool modified = false;
    std::vector<std::string> days = FrameworkUtils::string_split( p, "]" );
    if ( days.size() >= 7 )
    {
        for ( std::size_t d = 0; d < 7; d++ )
        {
            std::string day_str = FrameworkUtils::string_replace(FrameworkUtils::string_replace(FrameworkUtils::string_replace(days[d], "[", "" ), "\"", ""), ",", "");
            FrameworkUtils::string_tolower( day_str );
            for ( std::size_t h = 0; h < 24; h++ )
            {
                for ( std::size_t f = 0; f < 2; f++ )
                {
                    if ( day_str.length() > 0 )
                    {
                        char p = day_str[0];
                        day_str = day_str.substr( 1 );
                        ProgramType t = ERROR;
                        if ( p == 'x' )
                            t = HIGH_AUTO;
                        else if ( p == 'g' )
                            t = HIGH_GAS;
                        else if ( p == 'p' )
                            t = HIGH_PELLET;
                        else if ( p == 'm' )
                            t = LOW_PELLET;
                        else if ( p == 'o' )
                            t = LOW_GAS;

                        if ( t != ERROR )
                        {
                            if ( _program[d][h][f] != t )
                            {
                                modified = true;
                                _program[d][h][f] = t;
                            }
                        }
                    }
                } // f
            } //h
        } //d
    }
    return modified;
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
                        if ( token == "g" )
                            _program[d][h][f] = HIGH_GAS;
                        else if ( token == "x" )
                            _program[d][h][f] = HIGH_AUTO;
                        if ( token == "p" )
                            _program[d][h][f] = HIGH_PELLET;
                        else if ( token == "m" )
                            _program[d][h][f] = LOW_PELLET;
                        else
                            _program[d][h][f] = LOW_GAS;
                    }
                }
            }
        }
    }
}

void Program::saveConfig(ConfigData *c) const
{
    for ( std::size_t d = 0; d < 7; d++ )
    {
        std::string day_str = "day"+FrameworkUtils::utostring(d);
        std::string value = "";
        for ( std::size_t h = 0; h < 24; h++ )
        {
            for ( std::size_t f = 0; f < 2; f++ )
            {
                switch ( _program[d][h][f] )
                {
                case HIGH_AUTO:
                    value += "x";
                    break;
                case HIGH_GAS:
                    value += "g";
                    break;
                case HIGH_PELLET:
                    value += "p";
                    break;
                case LOW_PELLET:
                    value += "m";
                    break;
                case LOW_GAS:
                case ERROR:
                    value += "o";
                    break;
                }
                value += ",";
            }
        }
        c->setValue( day_str, value );
    }
}

void Program::writeJSON(FILE *file) const
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
                switch ( _program[d][h][f] )
                {
                case HIGH_AUTO:
                    s[1] = 'x';
                    break;
                case HIGH_GAS:
                    s[1] = 'g';
                    break;
                case HIGH_PELLET:
                    s[1] = 'p';
                    break;
                case LOW_PELLET:
                    s[1] = 'm';
                    break;
                case LOW_GAS:
                case ERROR:
                    s[1] = 'o';
                    break;
                }
                // disegna , solo alla fine della giornata:
                fwrite(s, ((h != 23) || (f != 1)) ? 4 : 3,1, file );
            }
        }
        // disegna , solo alla fine della lista:
        fwrite("],", (d != 6) ? 2 : 1, 1, file );
    }
    fwrite("]", 1, 1, file );
}
