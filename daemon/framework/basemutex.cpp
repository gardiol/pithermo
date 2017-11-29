#include "basemutex.h"

#include "debugprint.h"

using namespace FrameworkLibrary;

#include <pthread.h>

namespace FrameworkLibrary {

class __BaseMutexPrivate
{
    friend class BaseMutex;
    bool _locked;
    std::string _name;
    pthread_mutex_t _mutex;    
};

}

BaseMutex::BaseMutex(const std::string &name)
{
    // IMPORTANT NOTE: DO NOT USE THE MemoryChecker stuff in this class or you will
    // get a nice stack overflow!
    _private = new __BaseMutexPrivate;
    _private->_locked = false;
    _private->_name = name;
    pthread_mutex_init( &_private->_mutex, NULL );
}

BaseMutex::~BaseMutex()
{
    if ( _private->_locked )
        unlock();
    pthread_mutex_destroy( &_private->_mutex );
    delete _private;
    _private = NULL;
}

void BaseMutex::setName(const std::string &name)
{
    _private->_name = name;
}

void BaseMutex::lock()
{
    debugPrint( _private->_name, DebugPrint::MUTEX_CLASS ) << "Lock..\n";
    pthread_mutex_lock( &_private->_mutex );
    debugPrint( _private->_name, DebugPrint::MUTEX_CLASS ) << "..lockd\n";
    _private->_locked = true;
}

void BaseMutex::unlock()
{
    _private->_locked = false;
    debugPrint( _private->_name, DebugPrint::MUTEX_CLASS ) << "Unlock..\n";
    pthread_mutex_unlock( &_private->_mutex );
    debugPrint( _private->_name, DebugPrint::MUTEX_CLASS ) << "..unlockd\n";
}

bool BaseMutex::tryLock()
{
    return pthread_mutex_trylock( &_private->_mutex ) == 0;
}
