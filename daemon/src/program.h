#ifndef PROGRAM_H
#define PROGRAM_H

#include <vector>

#include "configfile.h"

using namespace FrameworkLibrary;

class Program
{
public:
    Program();
    ~Program();

    bool getPellet( int d, int h, int f );
    bool getGas( int d, int h, int f );

    bool change( const std::string& p );
    void loadConfig( const ConfigData* c );
    void saveConfig( ConfigData* c );

private:
    std::vector<std::vector<std::vector<bool> > > _gas_program;
    std::vector<std::vector<std::vector<bool> > > _pellet_program;
};

#endif // PROGRAM_H
