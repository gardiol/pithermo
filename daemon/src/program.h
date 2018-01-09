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

    void getProgram( int d, int h, int f,
                     bool& g, bool& p, bool &m );

    bool change( const std::string& p );
    void loadConfig( const ConfigData* c );
    void saveConfig( ConfigData* c );

    void writeJSON( FILE* file );

private:
    std::vector<std::vector<std::vector<bool> > > _gas_program;
    std::vector<std::vector<std::vector<bool> > > _pellet_program;
    std::vector<std::vector<std::vector<bool> > > _pellet_minimum_program;
};

#endif // PROGRAM_H
