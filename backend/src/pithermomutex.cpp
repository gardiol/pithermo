#include "pithermomutex.h"

#include <pthread.h>

class __PithermoMutexPrivate
{
    friend class PithermoMutex;
    bool _locked;
    std::string _name;
    pthread_mutex_t _mutex;    
};

PithermoMutex::PithermoMutex(const std::string &name)
{
    _private = new __PithermoMutexPrivate;
    _private->_locked = false;
    _private->_name = name;
    pthread_mutex_init( &_private->_mutex, NULL );
}

PithermoMutex::~PithermoMutex()
{
    if ( _private->_locked )
        unlock();
    pthread_mutex_destroy( &_private->_mutex );
    delete _private;
    _private = NULL;
}

void PithermoMutex::setName(const std::string &name)
{
    _private->_name = name;
}

void PithermoMutex::lock()
{
    pthread_mutex_lock( &_private->_mutex );
    _private->_locked = true;
}

void PithermoMutex::unlock()
{
    _private->_locked = false;
    pthread_mutex_unlock( &_private->_mutex );
}

bool PithermoMutex::tryLock()
{
    return pthread_mutex_trylock( &_private->_mutex ) == 0;
}
