#include "basesemaphor.h"

#include <pthread.h>

using namespace FrameworkLibrary;

class BaseSemaphor::__private_BaseSemaphor
{
public:
    __private_BaseSemaphor( const std::string& n ):
        _name(n),
        _signaled(false)
    {
        pthread_cond_init( &_cond, NULL );
        pthread_mutex_init( &_mutex, NULL );
    }

    ~__private_BaseSemaphor()
    {
        signal();
        pthread_cond_destroy( &_cond );
        pthread_mutex_destroy( &_mutex );
    }

    void wait()
    {
        pthread_mutex_lock( &_mutex );
        _signaled = false;
        while ( !_signaled )
            pthread_cond_wait( &_cond, &_mutex );
        pthread_mutex_unlock( &_mutex );
    }

    void signal()
    {
        pthread_mutex_lock( &_mutex );
        _signaled = true;
        pthread_cond_broadcast( &_cond );
        pthread_mutex_unlock( &_mutex );
    }

private:
    std::string _name;
    pthread_cond_t _cond;
    pthread_mutex_t _mutex;
    bool _signaled;

};

BaseSemaphor::BaseSemaphor(const std::string &name):
    _private( new __private_BaseSemaphor( name ) )
{

}

BaseSemaphor::~BaseSemaphor()
{
    delete _private;
    _private = NULL;
}

void BaseSemaphor::waitForSignal()
{
    return _private->wait();
}

void BaseSemaphor::signal()
{
    _private->signal();
}
