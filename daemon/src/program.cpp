#include "program.h"

#include <stdio.h>
#include <string.h>

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

    _template_names.resize( SharedStatusNumTemplates );
    _templates.resize( SharedStatusNumTemplates );
    for ( std::size_t t = 0; t < SharedStatusNumTemplates; t++ )
    {
        _template_names[t] = "";
        _templates[t].resize( 24 );
        for ( std::size_t h = 0; h < 24; h++ )
        {
            _templates[t][h].resize(2);
            for ( std::size_t f = 0; f < 2; f++ )
                _templates[t][h][f] = LOW_GAS;
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

bool Program::change(const std::string &new_program)
{
    bool modified = false;
    unsigned int pos = 0;
    for ( std::size_t d = 0; d < 7; d++ )
    {
        for ( std::size_t h = 0; h < 24; h++ )
        {
            for ( std::size_t f = 0; f < 2; f++ )
            {
                char p = new_program.at( pos++ );
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
            } // f
        } // h
    } //d
    return modified;
}

void Program::changeTemplate(unsigned int num, const std::string &name, const std::string &new_template)
{
    if ( num < SharedStatusNumTemplates )
    {
        unsigned int pos = 0;
        _template_names[num] = name;
        for ( std::size_t h = 0; h < 24; h++ )
        {
            for ( std::size_t f = 0; f < 2; f++ )
            {
                char p = new_template.at( pos++ );
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
                    _templates[num][h][f] = t;
                }
            } // f
        } // h
    }
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
                        else if ( token == "p" )
                            _program[d][h][f] = HIGH_PELLET;
                        else if ( token == "m" )
                            _program[d][h][f] = LOW_PELLET;
                        else
                            _program[d][h][f] = LOW_GAS;
                    }
                }
            }
        }

        for ( std::size_t t = 0; t < SharedStatusNumTemplates; t++ )
        {
            std::string templatenamesx = "template"+FrameworkUtils::utostring(t)+"_name";
            std::string templatex = "template"+FrameworkUtils::utostring(t);
            std::string t_string = c->getValue( templatex );
            std::vector<std::string> tokens = FrameworkUtils::string_split( t_string, ",");
            if ( tokens.size() >= 48 )
            {
                _template_names[t] = c->getValue( templatenamesx );
                std::size_t t = 0;
                for ( std::size_t h = 0; h < 24; h++ )
                {
                    for ( std::size_t f = 0; f < 2; f++ )
                    {
                        std::string token = tokens[t++];
                        FrameworkUtils::string_tolower( token );
                        if ( token == "g" )
                            _templates[t][h][f] = HIGH_GAS;
                        else if ( token == "x" )
                            _templates[t][h][f] = HIGH_AUTO;
                        else if ( token == "p" )
                            _templates[t][h][f] = HIGH_PELLET;
                        else if ( token == "m" )
                            _templates[t][h][f] = LOW_PELLET;
                        else
                            _templates[t][h][f] = LOW_GAS;
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

    for ( std::size_t t = 0; t < SharedStatusNumTemplates; t++ )
    {
        std::string template_name_str = "template"+FrameworkUtils::utostring(t)+"_name";
        std::string template_str = "template"+FrameworkUtils::utostring(t);
        std::string value = "";
        for ( std::size_t h = 0; h < 24; h++ )
        {
            for ( std::size_t f = 0; f < 2; f++ )
            {
                switch ( _templates[t][h][f] )
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
        c->setValue( template_str, value );
        c->setValue( template_name_str, _template_names[t] );
    }
}

void Program::writeRaw(SharedStatus *ss) const
{
    int pos = 0;
    for ( std::size_t d = 0; d < 7; d++ )
    {
        for ( std::size_t h = 0; h < 24; h++ )
        {
            for ( std::size_t f = 0; f < 2; f++ )
            {
                char c = 'o';
                switch ( _program[d][h][f] )
                {
                case HIGH_AUTO:
                    c = 'x';
                    break;
                case HIGH_GAS:
                    c = 'g';
                    break;
                case HIGH_PELLET:
                    c = 'p';
                    break;
                case LOW_PELLET:
                    c = 'm';
                    break;
                case LOW_GAS:
                case ERROR:
                    c = 'o';
                    break;
                }
                ss->program[pos++] = c;
            }
        }
    }

    for ( std::size_t t = 0; t < SharedStatusNumTemplates; t++ )
    {
        std::string tmp = _template_names[t];
        if ( tmp.length() > SharedStatusTemplatesNameSize -1 )
            tmp = tmp.substr(0, SharedStatusTemplatesNameSize-2 );
        memcpy(ss->templates_names[t], tmp.c_str(), tmp.length()+1 );
        int pos = 0;
        for ( std::size_t h = 0; h < 24; h++ )
        {
            for ( std::size_t f = 0; f < 2; f++ )
            {
                char c = 'o';
                switch ( _templates[t][h][f] )
                {
                case HIGH_AUTO:
                    c = 'x';
                    break;
                case HIGH_GAS:
                    c = 'g';
                    break;
                case HIGH_PELLET:
                    c = 'p';
                    break;
                case LOW_PELLET:
                    c = 'm';
                    break;
                case LOW_GAS:
                case ERROR:
                    c = 'o';
                    break;
                }
                ss->templates[t][pos++] = c;
            }
        }
    }
}
