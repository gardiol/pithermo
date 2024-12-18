#ifndef PITHERMOMUTEX_H
#define PITHERMOMUTEX_H

#include <string>

class __PithermoMutexPrivate;
class PithermoMutex
{
public:
    PithermoMutex( const std::string& name );
    virtual ~PithermoMutex();

    void setName( const std::string& name );
    void lock();
    void unlock();
    bool tryLock();

private:
    __PithermoMutexPrivate* _private;
};

#endif // PITHERMOMUTEX_H
