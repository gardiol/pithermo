#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#include <stdint.h>
#include <string>

class __private_ConfigData;
class ConfigFile
{
public:
    ConfigFile( std::string name, std::string file_data );
    ConfigFile( std::string filename );
    virtual ~ConfigFile();

    std::string toStr() const;

    bool saveToFile();

    std::string getName() const;
    std::string getFullName() const;
    void setName( const std::string& nn );

    bool isModified() const;

    bool isEmpty() const;

public: // Manage subsections, read only
    uint32_t getNumSections() const;
    ConfigFile* getSection( const std::string& name, uint32_t seq = 0 );
    uint32_t getSectionMulteplicity( const std::string& name );

public: // Manage subsections, write
    ConfigFile* newSection( const std::string& name );

    bool delSection( ConfigFile* subsection );

    void clearSections();

public:
    bool hasValue( const std::string& what ) const;
    std::string getValue( const std::string& what ) const;
    bool getValueBool( const std::string& what ) const;


public: // Values - modify content
    void addValue( const std::string& what, const std::string& value );
    void setValue( const std::string& what, const std::string& value );
    void setValueBool( const std::string& what, bool value );

protected:
    ConfigFile(const std::string& n, ConfigFile* parent);
    std::string _writeHeader() const;
    std::string _writeValues() const;
    std::string _writeSubSections() const;

private:
    void initialize(std::string file_data );
    void resetModified();

    __private_ConfigData* _private;
};




#endif // CONFIGFILE_H
