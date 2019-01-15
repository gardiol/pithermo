#ifndef PROGRAM_H
#define PROGRAM_H

#include <vector>

#include "sharedstatus.h"

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

    bool change(const std::string& new_program );
    void changeTemplate(unsigned int num, const std::string& name, const std::string& new_template );
    void loadConfig( const ConfigData* c );
    void saveConfig( ConfigData* c ) const;

    void writeRaw( SharedStatus* ss ) const;

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

    std::vector<std::string> _template_names;
    std::vector<std::vector<std::vector<ProgramType> > > _templates;
};

#endif // PROGRAM_H
