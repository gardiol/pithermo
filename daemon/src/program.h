#ifndef PROGRAM_H
#define PROGRAM_H

#include "configfile.h"

using namespace FrameworkLibrary;

class Program
{
public:
    Program( const ConfigData* c );
    ~Program();

    bool getPellet( int d, int h, int f );
    bool getGas( int d, int h, int f );
};

#endif // PROGRAM_H
