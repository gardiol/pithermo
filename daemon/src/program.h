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

    void setTime( int d, int h, int f );
    bool getGas() const
    {
        return _gas_program[_d][_h][_f];
    }
    bool getPellet() const
    {
        return _pellet_program[_d][_h][_f];
    }

    bool getPelletMinimum() const
    {
        return _pellet_minimum_program[_d][_h][_f];
    }

    bool change( const std::string& p );
    void loadConfig( const ConfigData* c );
    void saveConfig( ConfigData* c );

    void writeJSON( FILE* file );

private:
    int _d;
    int _h;
    int _f;
    std::vector<std::vector<std::vector<bool> > > _gas_program;
    std::vector<std::vector<std::vector<bool> > > _pellet_program;
    std::vector<std::vector<std::vector<bool> > > _pellet_minimum_program;
};

#endif // PROGRAM_H
