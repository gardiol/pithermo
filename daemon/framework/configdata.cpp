#include "configdata.h"

#include "memorychecker.h"

using namespace FrameworkLibrary;

namespace FrameworkLibrary {

class __private_ConfigData
{
    friend class ConfigData;
    std::list<std::string> _names;
    std::map<std::string, std::string> _values;
    std::map<std::string, std::vector<std::string> > _comments;
    std::list<ConfigData*> _subsections;
    std::string _name;
    bool _isModified;
    ConfigData* _parent;

};

}

ConfigData::ConfigData(const std::string& n, ConfigData *parent)
{    
    _private = new __private_ConfigData;
    _private->_name = n;
    _private->_comments.insert( std::pair<std::string, std::vector<std::string> >( "", std::vector<std::string>() ) );
    _private->_parent = parent;
    resetModified();
}

ConfigData* ConfigData::getParent()
{
    return _private->_parent;
}

std::string ConfigData::_writeHeader() const
{
    std::string buffer = "";
    std::vector<std::string> comments = getComments("");
    for ( std::vector<std::string>::iterator c = comments.begin(); c != comments.end(); ++c )
        buffer += "# " + (*c) + "\n";
    return buffer+"[" + getFullName() + "]\n";
}

std::string ConfigData::_writeValues() const
{
    std::string buffer = "";
    for ( std::list<std::string>::iterator i = _private->_names.begin(); i != _private->_names.end(); ++i )
    {
        std::string name = *i;
        std::string value = getValue( name );
        std::vector<std::string> comments = getComments( name );
        for ( std::vector<std::string>::iterator c = comments.begin(); c != comments.end(); ++c )
            buffer += "# " + (*c) + "\n";
        buffer += name + " = " + value + "\n";
    }
    return buffer;
}

std::string ConfigData::_writeSubSections() const
{
    std::string buffer = "";
    for ( std::list<ConfigData*>::const_iterator i = _private->_subsections.begin(),
          end = _private->_subsections.end(); i != end; ++i )
    {
        const ConfigData* subsection = *i;
        buffer += subsection->toStr();
        buffer += "\n";
    }
    return buffer;
}

void ConfigData::resetModified()
{
    _private->_isModified = false;
}

std::string ConfigData::toStr() const
{
    std::string buffer = _writeHeader();
    buffer += _writeValues();
    buffer += "\n";
    buffer += _writeSubSections();
    return buffer;
}

bool ConfigData::isModified() const
{
    return _private->_isModified;
}

bool ConfigData::isEmpty() const
{
    bool ns = getNumSections() > 0;
    bool nv = _private->_values.size() > 0;
    return !( ns || nv );
}

ConfigData::~ConfigData()
{
    for ( std::list<ConfigData*>::iterator i = _private->_subsections.begin(),
          end = _private->_subsections.end(); i != end; ++i )
    {
        ConfigData* subsection = *i;
        delete MEMORY_CHECKER_DEL( subsection, ConfigData );
    }
    _private->_parent = NULL;
    _private->_subsections.clear();
    delete _private;
    _private = NULL;
}

bool ConfigData::hasValue( const std::string& what ) const
{
    return _private->_values.find( what ) != _private->_values.end();
}

void ConfigData::addValue( const std::string& what, const std::string& value )
{
    std::string w = what;
    if ( hasValue( w ) )
    {
        if ( getValue( w ) == value )
            return;
        removeValue( w );
    }
    _private->_isModified = true;
    _private->_names.push_back( w );
    _private->_values.insert( std::pair<std::string, std::string>(w, value) );
    _private->_comments.insert( std::pair<std::string, std::vector<std::string> >( w, std::vector<std::string>() ) );
}

void ConfigData::setValue(const std::string &what, const std::string &value)
{
    addValue( what, value );
}

void ConfigData::setValueBool(const std::string &what, bool value)
{
    setValue( what, value ? "true" : "false" );
}

void ConfigData::removeValue( const std::string& what )
{
    std::string w = what;
    if ( _private->_values.find( w ) != _private->_values.end() )
    {
        _private->_isModified = true;
        _private->_values.erase( w );
    }
    if ( _private->_comments.find( w ) != _private->_comments.end() )
    {
        _private->_isModified = true;
        _private->_comments.erase( w );
    }
    for ( std::list<std::string>::iterator n = _private->_names.begin(); n != _private->_names.end(); ++n )
    {
        if ( (*n) == w )
        {
            _private->_isModified = true;
            _private->_names.erase( n );
            break;
        }
    }
}

void ConfigData::clearValues()
{
    std::list<std::string> names = _private->_names;
    for ( std::list<std::string>::iterator i = names.begin(); i != names.end(); ++i )
        removeValue( *i );
}

std::string ConfigData::getValue(const std::string &what) const
{
    std::string retval = "";
    std::string  w = /*FrameworkUtils::string_tolower(*/what/*)*/;
    if ( _private->_values.find( w ) != _private->_values.end() )
        retval = _private->_values.at( w );
    return retval;
}

std::map<std::string, std::string> ConfigData::getValues( const std::string& prefix ) const
{
    std::map<std::string, std::string> ret;
    int prefix_len = prefix.length();
    for ( std::map<std::string, std::string>::const_iterator i = _private->_values.begin(); i != _private->_values.end(); ++i )
    {
        std::string nm = i->first;
        std::string vl = i->second;
        if ( (prefix_len == 0) || ( nm.substr(0, prefix_len) == prefix ) )
            ret.insert( std::pair<std::string, std::string>( nm.substr( prefix_len ), vl) );
    }
    return ret;
}

bool ConfigData::getValueBool(const std::string &what) const
{
    std::string value = /*FrameworkUtils::string_tolower(*/ getValue( what ) /*)*/;
    return ( value == "true" ) ||
            (value == "yes" ) ||
            (value == "si" ) ||
            (value == "1" );
}

std::string ConfigData::getName() const
{
    return _private->_name;
}

std::string ConfigData::getFullName() const
{
    // We must skip the top-level name (the filename) is not part of the full-name:
    if ( _private->_parent != NULL )
    {
        if ( _private->_parent->getParent() != NULL )
            return _private->_parent->getFullName() + ":" + _private->_name;
    }
    return _private->_name;
}

void ConfigData::setName( const std::string& nn )
{
    if ( _private->_name != nn )
    {
        _private->_isModified = true;
        _private->_name = nn;
    }
}

/*std::vector<std::string> ConfigData::getNames() const
{
    std::vector<std::string> _names;
    for ( std::map<std::string, std::string>::const_iterator i = _private->_values.begin(); i != _private->_values.end(); ++i )
    {
        std::string nm = i->first;
        _names.push_back( nm );
    }
    return _names;
}*/

std::list<std::string> ConfigData::listNames() const
{
    return _private->_names;
}

void ConfigData::clearComments(const std::string &what)
{
    if ( (what == "") || hasValue(what) )
    {
        std::map<std::string, std::vector<std::string> >::iterator x = _private->_comments.find( what );
        if ( x != _private->_comments.end() )
        {
            _private->_isModified = true;
            _private->_comments[ what ].clear();
        }
    }
}

std::vector<std::string> ConfigData::getComments(const std::string &what) const
{
    std::string what_lower = what;

    if ( (what_lower == "") || hasValue(what_lower) )
    {
        std::map<std::string, std::vector<std::string> >::iterator x = _private->_comments.find( what_lower );
        if ( x != _private->_comments.end() )
            return x->second;
    }
    return std::vector<std::string>();
}

void ConfigData::setComments(const std::string &what, const std::vector<std::string> &new_comments)
{
    std::string what_lower = what;

    if ( (what_lower == "") || hasValue(what_lower) )
    {
        if ( _private->_comments.find( what_lower ) != _private->_comments.end() )
        {
            if ( _private->_comments[ what_lower ] == new_comments )
                return;
        }
        _private->_isModified = true;
        _private->_comments[ what_lower ] = new_comments;
    }
}

uint32_t ConfigData::getNumSections() const
{
    return _private->_subsections.size();
}

std::list<ConfigData*> ConfigData::listSections() const
{
    return _private->_subsections;
}

ConfigData *ConfigData::getSection(const std::string &name, uint32_t seq)
{
    for ( std::list<ConfigData*>::const_iterator i = _private->_subsections.begin(),
          end = _private->_subsections.end(); i != end; ++i )
    {
        ConfigData* subsection = *i;
        if ( subsection->getName() == name )
        {
            if ( seq == 0 )
                return subsection;
            else
                seq--;
        }
    }
    return NULL;
}

uint32_t ConfigData::getSectionMulteplicity(const std::string &name)
{
    uint32_t ret = 0;
    for ( std::list<ConfigData*>::const_iterator i = _private->_subsections.begin(),
          end = _private->_subsections.end(); i != end; ++i )
    {
        ConfigData* subsection = *i;
        if ( subsection->getName() == name )
            ret++;
    }
    return ret;
}

ConfigData* ConfigData::newSection(const std::string &name)
{
    ConfigData* new_section = MEMORY_CHECKER_ADD( new ConfigData( name, this ), ConfigData );
    _private->_isModified = true;
    _private->_subsections.push_back( new_section );
    return new_section;
}

bool ConfigData::delSection(ConfigData *subsection)
{
    for ( std::list<ConfigData*>::iterator i = _private->_subsections.begin(),
          end = _private->_subsections.end(); i != end; ++i )
    {
        if ( *i == subsection )
        {
            _private->_isModified = true;
            _private->_subsections.erase( i );
            delete MEMORY_CHECKER_DEL(subsection, ConfigData);
            return true;
        }
    }
    return false;
}

void ConfigData::clearSections()
{
    _private->_isModified = true;
    for ( std::list<ConfigData*>::iterator i = _private->_subsections.begin(),
          end = _private->_subsections.end(); i != end; ++i )
        delete MEMORY_CHECKER_DEL(*i, ConfigData);
    _private->_subsections.clear();
}
