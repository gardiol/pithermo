#ifndef PITHERMOSIGHANDLER_H
#define PITHERMOSIGHANDLER_H

#include <stdint.h>

class PithermoSignal;
class PithermoSigHandler
{
    friend class __privateSighandler;
public:
    static bool setHandler( PithermoSignal* handler, int32_t user_defined_signal = -1 );
    static bool unsetHandler( PithermoSignal* handler, int32_t user_defined_signal = -1 );

private:
    PithermoSigHandler();
};

class PithermoSignal
{
    friend class __privateSighandler;

public:
    virtual ~PithermoSignal()
    {
    }

protected:
    virtual void customHandler(int32_t user_defined_signal ) = 0;

};

#endif
