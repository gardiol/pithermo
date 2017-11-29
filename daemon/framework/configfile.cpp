#include "configfile.h"

#include "frameworkutils.h"

using namespace FrameworkLibrary;

ConfigFile::ConfigFile(std::string name, std::string file_data):
    ConfigData( name, NULL )
{
    std::vector<std::string> comments;
    comments.reserve(10);
    // To read also last row if the file does not end by \n:
    file_data += "\n";

    ConfigData* new_section = this;
    std::string::size_type pos = file_data.find_first_of('\n');
    while ( pos != std::string::npos )
    {
        std::string row = file_data.substr( 0, pos  );
        file_data = file_data.substr( pos+1 );

        // Strip leading and trailing spaces:
        FrameworkUtils::string_trim( row );

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
                FrameworkUtils::string_trim( section_title );
                new_section = this;
                std::vector<std::string> names = FrameworkUtils::string_split( section_title, ":" );
                for ( unsigned int n = 0; (new_section != NULL) && (n < (names.size()) ); n++ )
                {
                    ConfigData* tmp_section = new_section->getSection( names[n] );
                    if ( (tmp_section == NULL) || (n == names.size()-1) )
                        tmp_section = new_section->newSection( names[n] );
                    new_section = tmp_section;
                }
                if ( new_section != NULL )
                {
                    new_section->setComments( "", comments );
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
                    FrameworkUtils::string_trim( what );
                    FrameworkUtils::string_trim( value );
                    if ( what != "" )
                    {
                        new_section->addValue( what, value );
                        new_section->setComments( what, comments );
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

ConfigFile::~ConfigFile()
{
}

std::string ConfigFile::toStr() const
{
    std::string buffer = _writeValues();
    buffer += "\n";
    buffer += _writeSubSections();
    return buffer;
}
