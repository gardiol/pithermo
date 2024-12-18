#include "program.h"

#include "pithermoutils.h"
#include "configfile.h"

#include <string.h>

Program::Program():
    _d(0),
    _h(0),
    _f(0)
{
    _program.resize(7);
    for ( std::size_t d = 0; d < 7; d++ )
        _program[d] = "oooooooooooooooooooooooooooooooooooooooooooooooo";

    _template_names.resize( SharedStatusNumTemplates );
    _templates.resize( SharedStatusNumTemplates );
    for ( std::size_t t = 0; t < SharedStatusNumTemplates; t++ )
    {
        _template_names[t] = "";
        _templates[t] = "oooooooooooooooooooooooooooooooooooooooooooooooo";
    }
}

Program::~Program()
{

}

bool Program::useHigh() const
{
    char p = _program[_d].at( _h*2+_f );
    return p == 'x' || p == 'g' || p == 'p';
}

bool Program::usePellet() const
{
    char p = _program[_d].at( _h*2+_f );
    return p == 'x' || p == 'm' || p == 'p';
}

bool Program::useGas() const
{
    char p = _program[_d].at( _h*2+_f );
    return p == 'x' || p == 'g' || p == 'o';
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
    if ( new_program.length() == 48*7 )
    {
        for ( std::size_t d = 0; d < 7; d++ )
        {
            std::string day_string = new_program.substr( d*48, 48 );
            if ( day_string != _program[d] )
            {
                modified = true;
                _program[d] = day_string;
            }
        } //d
    }
    return modified;
}

void Program::changeTemplate(unsigned int num, const std::string &name, const std::string &new_template)
{
    if ( num < SharedStatusNumTemplates )
    {
        _template_names[num] = name;
        if ( new_template.length() == 48 )
        {
            _templates[num] = new_template;
        }
    }
}

void Program::loadConfig(const ConfigFile *c)
{
    if ( c != nullptr )
    {
        for ( std::size_t d = 0; d < 7; d++ )
        {
            std::string day = "day"+PithermoUtils::utostring(d);
            std::string day_string = c->getValue( day );
            // convert old format
            day_string = PithermoUtils::string_replace( day_string, ",", "");
            if ( day_string.length() == 48 )
            {
                _program[d] = day_string;
            }
        }

        for ( std::size_t t = 0; t < SharedStatusNumTemplates; t++ )
        {
            std::string templatenamesx = "template"+PithermoUtils::utostring(t)+"_name";
            std::string templatex = "template"+PithermoUtils::utostring(t);
            std::string t_string = c->getValue( templatex );
            _template_names[t] = c->getValue( templatenamesx );
            if ( t_string.length() == 48 )
            {
                _templates[t] = t_string;
            }
        }
    }
}

void Program::saveConfig(ConfigFile *c) const
{
    for ( std::size_t d = 0; d < 7; d++ )
    {
        std::string day_str = "day"+PithermoUtils::utostring(d);
        c->setValue( day_str, _program[d] );
    }

    for ( std::size_t t = 0; t < SharedStatusNumTemplates; t++ )
    {
        std::string template_name_str = "template"+PithermoUtils::utostring(t)+"_name";
        std::string template_str = "template"+PithermoUtils::utostring(t);
        c->setValue( template_str, _templates[t] );
        c->setValue( template_name_str, _template_names[t] );
    }
}

void Program::writeRaw(SharedStatus *ss) const
{
    int pos = 0;
    for ( std::size_t d = 0; d < 7; d++ )
    {
        memcpy( &ss->program[pos], _program[d].c_str(), 48 );
        pos+=48;
    }

    for ( std::size_t t = 0; t < SharedStatusNumTemplates; t++ )
    {
        std::string tmp = _template_names[t];
        if ( tmp.length() > SharedStatusTemplatesNameSize -1 )
            tmp = tmp.substr(0, SharedStatusTemplatesNameSize-2 );
        memcpy(ss->templates_names[t], tmp.c_str(), tmp.length()+1 );
        memcpy(ss->templates[t], _templates[t].c_str(), 48 );
    }
}
