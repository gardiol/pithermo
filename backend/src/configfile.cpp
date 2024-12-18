#include "configfile.h"

#include "pithermoutils.h"

#include <map>

class __private_ConfigData
{
    friend class ConfigFile;
    std::list<std::string> _names;
    std::map<std::string, std::string> _values;
    std::list<ConfigFile*> _subsections;
    std::string _name;
    bool _isModified;
    ConfigFile* _parent;
    bool _isDisabled;
};

ConfigFile::ConfigFile(std::string name, std::string file_data):
    ConfigFile( name, nullptr )
{
    initialize( file_data );
}

ConfigFile::ConfigFile(std::string filename):
    ConfigFile( filename, nullptr )
{
    std::string config_file_content = "";
    PithermoUtils::file_to_str( filename, config_file_content );
    initialize( config_file_content );
}

ConfigFile::~ConfigFile()
{
    for ( std::list<ConfigFile*>::iterator i = _private->_subsections.begin(),
         end = _private->_subsections.end(); i != end; ++i )
    {
        ConfigFile* subsection = *i;
        delete subsection;
    }
    _private->_parent = NULL;
    _private->_subsections.clear();
    delete _private;
    _private = NULL;
}

class __private_ConfigFile
{
    friend class ConfigFile;
    std::list<std::string> _names;
    std::map<std::string, std::string> _values;
    std::map<std::string, std::vector<std::string> > _comments;
    std::list<ConfigFile*> _subsections;
    std::string _name;
    bool _isModified;
    ConfigFile* _parent;

};

ConfigFile::ConfigFile(const std::string& n, ConfigFile *parent)
{
    _private = new __private_ConfigData;
    _private->_name = n;
    _private->_parent = parent;
    resetModified();
}

std::string ConfigFile::_writeHeader() const
{
    return "[" + getFullName() + "]\n";
}

std::string ConfigFile::_writeValues() const
{
    std::string buffer = "";
    for ( std::list<std::string>::iterator i = _private->_names.begin(); i != _private->_names.end(); ++i )
    {
        std::string name = *i;
        std::string value = getValue( name );
        buffer += name + " = " + value + "\n";
    }
    return buffer;
}

std::string ConfigFile::_writeSubSections() const
{
    std::string buffer = "";
    for ( std::list<ConfigFile*>::const_iterator i = _private->_subsections.begin(),
         end = _private->_subsections.end(); i != end; ++i )
    {
        const ConfigFile* subsection = *i;
        buffer += subsection->toStr();
        buffer += "\n";
    }
    return buffer;
}

void ConfigFile::resetModified()
{
    _private->_isModified = false;
}

std::string ConfigFile::toStr() const
{
    std::string buffer = _writeValues();
    buffer += "\n";
    buffer += _writeSubSections();
    return buffer;
}


bool ConfigFile::isModified() const
{
    return _private->_isModified;
}

bool ConfigFile::isEmpty() const
{
    bool ns = getNumSections() > 0;
    bool nv = _private->_values.size() > 0;
    return !( ns || nv );
}


bool ConfigFile::hasValue( const std::string& what ) const
{
    return _private->_values.find( what ) != _private->_values.end();
}

void ConfigFile::addValue( const std::string& what, const std::string& value )
{
    std::string w = what;
    if ( hasValue( w ) )
    {
        if ( getValue( w ) == value )
            return;
        _private->_values.erase( _private->_values.find( what ) );
    }
    _private->_isModified = true;
    _private->_names.push_back( w );
    _private->_values.insert( std::pair<std::string, std::string>(w, value) );
}

void ConfigFile::setValue(const std::string &what, const std::string &value)
{
    addValue( what, value );
}

void ConfigFile::setValueBool(const std::string &what, bool value)
{
    setValue( what, value ? "true" : "false" );
}

/*void ConfigFile::removeValue( const std::string& what )
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

void ConfigFile::clearValues()
{
    std::list<std::string> names = _private->_names;
    for ( std::list<std::string>::iterator i = names.begin(); i != names.end(); ++i )
        removeValue( *i );
}*/

std::string ConfigFile::getValue(const std::string &what) const
{
    std::string retval = "";
    if ( _private->_values.find( what ) != _private->_values.end() )
        retval = _private->_values.at( what );
    return retval;
}

bool ConfigFile::getValueBool(const std::string &what) const
{
    std::string value = PithermoUtils::string_tolower( getValue( what ) );
    return ( value == "true" ) ||
           (value == "yes" ) ||
           (value == "si" ) ||
           (value == "1" );
}

std::string ConfigFile::getName() const
{
    return _private->_name;
}

std::string ConfigFile::getFullName() const
{
    // We must skip the top-level name (the filename) is not part of the full-name:
    if ( _private->_parent != NULL )
    {
        if ( _private->_parent->_private->_parent != NULL )
            return _private->_parent->getFullName() + ":" + _private->_name;
    }
    return _private->_name;
}

void ConfigFile::setName( const std::string& nn )
{
    if ( _private->_name != nn )
    {
        _private->_isModified = true;
        _private->_name = nn;
    }
}

uint32_t ConfigFile::getNumSections() const
{
    return _private->_subsections.size();
}

ConfigFile *ConfigFile::getSection(const std::string &name, uint32_t seq)
{
    for ( std::list<ConfigFile*>::const_iterator i = _private->_subsections.begin(),
         end = _private->_subsections.end(); i != end; ++i )
    {
        ConfigFile* subsection = *i;
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

uint32_t ConfigFile::getSectionMulteplicity(const std::string &name)
{
    uint32_t ret = 0;
    for ( std::list<ConfigFile*>::const_iterator i = _private->_subsections.begin(),
         end = _private->_subsections.end(); i != end; ++i )
    {
        ConfigFile* subsection = *i;
        if ( subsection->getName() == name )
            ret++;
    }
    return ret;
}

ConfigFile* ConfigFile::newSection(const std::string &name)
{
    ConfigFile* new_section = new ConfigFile( name, this );
    _private->_isModified = true;
    _private->_subsections.push_back( new_section );
    return new_section;
}

bool ConfigFile::delSection(ConfigFile *subsection)
{
    for ( std::list<ConfigFile*>::iterator i = _private->_subsections.begin(),
         end = _private->_subsections.end(); i != end; ++i )
    {
        if ( *i == subsection )
        {
            _private->_isModified = true;
            _private->_subsections.erase( i );
            delete subsection;
            return true;
        }
    }
    return false;
}

void ConfigFile::clearSections()
{
    _private->_isModified = true;
    for ( std::list<ConfigFile*>::iterator i = _private->_subsections.begin(),
         end = _private->_subsections.end(); i != end; ++i )
        delete *i;
    _private->_subsections.clear();
}

bool ConfigFile::saveToFile()
{
    std::string config_file = getName();
    return PithermoUtils::str_to_file( config_file, toStr() );
}

void ConfigFile::initialize(std::string file_data)
{
    std::vector<std::string> comments;
    comments.reserve(10);
    // To read also last row if the file does not end by \n:
    file_data += "\n";

    ConfigFile* new_section = this;
    std::string::size_type pos = file_data.find_first_of('\n');
    while ( pos != std::string::npos )
    {
        std::string row = file_data.substr( 0, pos  );
        file_data = file_data.substr( pos+1 );

        // Strip leading and trailing spaces:
        PithermoUtils::string_trim( row );

        // Does this line contains a comment?
        std::string::size_type comment_pos = row.find_first_of('#');
        if ( comment_pos != std::string::npos )
            comments.push_back( row.substr( comment_pos+1 ) );

        row = row.substr( 0, comment_pos );
        if ( row.length() > 0 )
        {
            // Inizio / fine sezione?
            if ( row[0] == '[' )
            {
                std::string section_title = row.substr( 1, row.find_last_of(']')-1 );
                PithermoUtils::string_trim( section_title );
                new_section = this;
                std::vector<std::string> names = PithermoUtils::string_split( section_title, ":" );
                for ( unsigned int n = 0; (new_section != NULL) && (n < (names.size()) ); n++ )
                {
                    ConfigFile* tmp_section = new_section->getSection( names[n] );
                    if ( (tmp_section == NULL) || (n == names.size()-1) )
                        tmp_section = new_section->newSection( names[n] );
                    new_section = tmp_section;
                }
                if ( new_section != NULL )
                {
                    new_section->resetModified();
                    comments.clear();
                }
            }
            else
            {
                if ( new_section != NULL )
                {
                    // Leggi cosa e valore:
                    int delimiter_pos = row.find_first_of('=');
                    std::string what = row.substr( 0, delimiter_pos );
                    std::string value = row.substr( delimiter_pos+1 );
                    // Trimma spazi:
                    PithermoUtils::string_trim( what );
                    PithermoUtils::string_trim( value );
                    if ( what != "" )
                    {
                        new_section->addValue( what, value );
                        new_section->resetModified();
                        comments.clear();
                    }
                }
            }
        }
        pos = file_data.find_first_of('\n');
    }
    resetModified();

}
