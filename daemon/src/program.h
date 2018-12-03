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

    bool useHigh() const;
    bool usePellet() const;
    bool useGas() const;

    void setTime( int d, int h, int f );

    bool change( const std::string& p );
    void loadConfig( const ConfigData* c );
    void saveConfig( ConfigData* c ) const;

    void writeJSON( FILE* file ) const;

private:
    enum ProgramType { LOW_GAS,
                       LOW_PELLET,
                       HIGH_GAS,
                       HIGH_PELLET,
                       HIGH_AUTO,
                       ERROR};

    std::size_t _d;
    std::size_t _h;
    std::size_t _f;
    std::vector<std::vector<std::vector<ProgramType> > > _program;
};

#endif // PROGRAM_H
