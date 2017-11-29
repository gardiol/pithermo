#include "basethread.h"
#include "debugprint.h"

#if defined(FRAMEWORK_PLATFORM_WINDOWS)
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    #include <pthread.h>
    #include <unistd.h>
    #include <sys/syscall.h>
#endif

#include "memorychecker.h"
#include "frameworkutils.h"
#include "basemutex.h"

#include <stdio.h>

using namespace FrameworkLibrary;

namespace FrameworkLibrary {

class __BaseThreadPrivate
{
    friend class BaseThread;

    explicit __BaseThreadPrivate( const std::string& name ):
        _start_mutex( "" ),
        _terminateRequest_mutex( "" )
    {
        setName( name );
    }

    void setName( const std::string& name )
    {
        _name = name;
        _start_mutex.setName( name + "_start" );
        _terminateRequest_mutex.setName( name + "_terminate_request" );
    }

    int join_value;
    BaseMutex _start_mutex;
    bool running;
    FrameworkTimer _timer;
    std::string _name;

    bool _terminateRequest;
    BaseMutex _terminateRequest_mutex;

#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    HANDLE thread_id;
    static DWORD WINAPI staticRun( LPVOID arg )
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    pthread_t thread_id;    
    static void* staticRun(void* arg)
#endif
    {
        BaseThread* myself = static_cast<BaseThread*>(arg);
        if ( myself != NULL )
        {
            debugPrint( myself->_pdata->_name, DebugPrint::THREAD_CLASS ) << "New thread started...\n";
            myself->_pdata->_timer.start();
            myself->run();
            myself->_pdata->_timer.stop();
            myself->_pdata->running = false;
            debugPrint( myself->_pdata->_name, DebugPrint::THREAD_CLASS ) << "Thread stopped.\n";
        }
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
        return 0;
#elif defined(FRAMEWORK_PLATFORM_LINUX)
        return NULL;
#endif
    }

    static std::set<BaseThread*> _threads;
    static BaseMutex _threads_mutex;
};

std::set<BaseThread*> __BaseThreadPrivate::_threads;
BaseMutex __BaseThreadPrivate::_threads_mutex("threads_mutex");

}

void BaseThread::printStatistics()
{
    debugPrintUntagged() << "**** BaseThread: thread status and statistics: *****\n";
    __BaseThreadPrivate::_threads_mutex.lock();
    for ( std::set<BaseThread*>::iterator t = __BaseThreadPrivate::_threads.begin();
          t != __BaseThreadPrivate::_threads.end();
          ++t )
    {
        BaseThread* thread = *t;
        debugPrintUntagged() << "Thread '" << thread->getName() << "' is " << (thread->isRunning() ? "running" : "stopped") << "! " << (thread->terminationRequested() ? " Termination has been requested." : "") << "\n";
        thread->customPrintStatistics();
    }
    __BaseThreadPrivate::_threads_mutex.unlock();
}

BaseThread::BaseThread(const std::string &name)
{
    __BaseThreadPrivate::_threads_mutex.lock();
    __BaseThreadPrivate::_threads.insert( this );
    __BaseThreadPrivate::_threads_mutex.unlock();
    _pdata = new __BaseThreadPrivate( name );
    _pdata->join_value = -1;
    _pdata->thread_id = 0;
    _pdata->running = false;
    _pdata->_terminateRequest = false;
}

BaseThread::~BaseThread()
{
    requestTerminate();
    waitForEnd();
    delete _pdata;
    _pdata = NULL;
    __BaseThreadPrivate::_threads_mutex.lock();
    __BaseThreadPrivate::_threads.erase( this );
    __BaseThreadPrivate::_threads_mutex.unlock();
}

void BaseThread::setName(const std::string &name)
{
    _pdata->setName( name );
}

BaseThreadId BaseThread::getThreadId()
{
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    return GetCurrentThreadId();
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    return (BaseThreadId)syscall(SYS_gettid);
#endif
}

bool BaseThread::isRunning() const
{
    return _pdata->running;
}

std::string BaseThread::getName() const
{
    return _pdata->_name;
}

int BaseThread::waitForEnd()
{
    if ( _pdata->thread_id != 0 )
    {
        debugPrint( _pdata->_name, DebugPrint::THREAD_CLASS ) << "Waiting for thread to stop...\n";
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
        WaitForSingleObject( _pdata->thread_id, INFINITE );
#elif defined(FRAMEWORK_PLATFORM_LINUX)
        pthread_join( _pdata->thread_id, NULL );
#endif
        _pdata->thread_id = 0;
    }
    return _pdata->join_value;
}

void BaseThread::requestTerminate()
{
    _pdata->_terminateRequest_mutex.lock();
    debugPrint( _pdata->_name, DebugPrint::THREAD_CLASS ) << "Termination requested!\n";
    _pdata->_terminateRequest = true;
    _pdata->_terminateRequest_mutex.unlock();
}

uint64_t BaseThread::elapsedTimeMs() const
{
    return _pdata->_timer.elapsedTimeMS();
}

void BaseThread::startThread()
{
    _pdata->_start_mutex.lock();
    if ( !isRunning() )
    {
        _pdata->running = true;

        _pdata->_terminateRequest_mutex.lock();
        _pdata->_terminateRequest = false;
        _pdata->_terminateRequest_mutex.unlock();

#if defined(FRAMEWORK_PLATFORM_WINDOWS)
        if ( (_pdata->thread_id = CreateThread(NULL,
                          0,
                          __BaseThreadPrivate::staticRun,
                          (void*)this,
                          0,
                          NULL )) == NULL )
#elif defined(FRAMEWORK_PLATFORM_LINUX)
        if ( pthread_create( &(_pdata->thread_id), NULL, __BaseThreadPrivate::staticRun, (void*)this) != 0 )
#endif
        {
            _pdata->running = false;
            _pdata->thread_id = 0;
            debugPrintError( _pdata->_name ) << "Error creating thread: " << FrameworkUtils::get_errno_string("") << "\n";
        }
    }
    _pdata->_start_mutex.unlock();
}

bool BaseThread::terminationRequested() const
{
    _pdata->_terminateRequest_mutex.lock();
    bool ret = _pdata->_terminateRequest;
    _pdata->_terminateRequest_mutex.unlock();
    return ret;
}

void BaseThread::setReturnValue(int val)
{
    _pdata->join_value = val;
}

void BaseThread::customPrintStatistics()
{
}
