#include "pithermothread.h"

#include "pithermomutex.h"
#include "pithermotimer.h"

#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>

#include <set>

class PithermoThread::__PithermoThreadPrivate
{
    friend class PithermoThread;

    explicit __PithermoThreadPrivate( const std::string& name ):
        _start_mutex( "" ),
        _terminateRequest_mutex( "" )
    {
        _freq_us = 0;
        setName( name );
    }

    void setName( const std::string& name )
    {
        _name = name;
        _start_mutex.setName( name + "_start" );
        _terminateRequest_mutex.setName( name + "_terminate_request" );
    }

    int join_value;
    PithermoMutex _start_mutex;
    bool running;
    std::string _name;

    bool _terminateRequest;
    PithermoMutex _terminateRequest_mutex;

    pthread_t thread_id;    
    static void* staticRun(void* arg)
    {
        PithermoThread* myself = static_cast<PithermoThread*>(arg);
        if ( myself != NULL )
        {
            myself->run();
            myself->_private->running = false;
        }
        return NULL;
    }

    uint64_t _freq_us;

    static std::set<PithermoThread*> _threads;
    static PithermoMutex _threads_mutex;
};


std::set<PithermoThread*> PithermoThread::__PithermoThreadPrivate::_threads;
PithermoMutex PithermoThread::__PithermoThreadPrivate::_threads_mutex("threads_mutex");

PithermoThread::PithermoThread(const std::string &name,  uint64_t freq_us):
    _private( new __PithermoThreadPrivate( name ) )
{
    __PithermoThreadPrivate::_threads_mutex.lock();
    __PithermoThreadPrivate::_threads.insert( this );
    __PithermoThreadPrivate::_threads_mutex.unlock();
    _private->_freq_us = freq_us;
    _private->join_value = -1;
    _private->thread_id = 0;
    _private->running = false;
    _private->_terminateRequest = false;
}

PithermoThread::~PithermoThread()
{
    requestTerminate();
    waitForEnd();
    delete _private;
    _private = NULL;
    __PithermoThreadPrivate::_threads_mutex.lock();
    __PithermoThreadPrivate::_threads.erase( this );
    __PithermoThreadPrivate::_threads_mutex.unlock();
}


void PithermoThread::setName(const std::string &name)
{
    _private->setName( name );
}

BaseThreadId PithermoThread::getThreadId()
{
    return (BaseThreadId)syscall(SYS_gettid);
}

bool PithermoThread::isRunning() const
{
    return _private->running;
}

std::string PithermoThread::getName() const
{
    return _private->_name;
}

int PithermoThread::waitForEnd()
{
    if ( _private->thread_id != 0 )
    {
        pthread_join( _private->thread_id, NULL );
        _private->thread_id = 0;
    }
    return _private->join_value;
}

void PithermoThread::requestTerminate()
{
    _private->_terminateRequest_mutex.lock();
    _private->_terminateRequest = true;
    _private->_terminateRequest_mutex.unlock();
}

void PithermoThread::startThread()
{
    _private->_start_mutex.lock();
    if ( !isRunning() )
    {
        _private->running = true;

        _private->_terminateRequest_mutex.lock();
        _private->_terminateRequest = false;
        _private->_terminateRequest_mutex.unlock();

        if ( pthread_create( &(_private->thread_id), NULL, __PithermoThreadPrivate::staticRun, (void*)this) != 0 )
        {
            _private->running = false;
            _private->thread_id = 0;
        }
    }
    _private->_start_mutex.unlock();
}

bool PithermoThread::terminationRequested() const
{
    _private->_terminateRequest_mutex.lock();
    bool ret = _private->_terminateRequest;
    _private->_terminateRequest_mutex.unlock();
    return ret;
}

void PithermoThread::setReturnValue(int val)
{
    _private->join_value = val;
}

void PithermoThread::setFrequency(uint64_t freq_us)
{
    if ( !isRunning() )
        _private->_freq_us = freq_us;
}

bool PithermoThread::scheduleStart()
{
    return true;
}

void PithermoThread::schedulingStopped()
{

}

void PithermoThread::run()
{
    if ( _private->_freq_us > 0 )
    {
        if ( scheduleStart() )
        {
            PithermoTimer run_timer;
            bool run_ok = true;
            uint64_t cycle = 0;
            run_timer.setLoopTime( _private->_freq_us );
            while ( !terminationRequested() && run_ok )
            {
                run_ok = scheduledRun( run_timer.elapsedTime() , cycle++ );
                run_timer.waitLoop();
            }
            schedulingStopped();
        }
    }
}
