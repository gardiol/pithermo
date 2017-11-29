#include "frameworktimer.h"
#include "basemutex.h"
#include "debugprint.h"
#include "memorychecker.h"
#include "profiler.h"

#include <errno.h>
#include <stdio.h>

using namespace FrameworkLibrary;

namespace FrameworkLibrary {

class FrameworkTimer::__FrameworkTimerPrivate
{
    friend class FrameworkTimer;

    static int64_t usleep( uint64_t usecs );

    uint64_t _start_time;
    uint64_t _stop_time;
    uint64_t _loop_time;
    uint64_t _loop_interval;
    bool _quiet;
    bool _running;
    bool _paused;
};

#if defined(FRAMEWORK_PLATFORM_WINDOWS)

#include <mmsystem.h>

class __staticWindowsTimerStuff_cbData
{
public:
    __staticWindowsTimerStuff_cbData():
        mutex("timer_mutex")
    {
    }

    BaseMutex mutex;
    uint32_t usWait;
};

// This class implements the Windows Multimedia Timers to achieve 1ms precision sleep
// It requires the multimedia thread and a double-lock to operate without
// overloading the CPU with spinlocks.
class __staticWindowsTimerStuff
{
public:
    static __staticWindowsTimerStuff* initialize()
    {
        static __staticWindowsTimerStuff swts;
        return &swts;
    }

    ~__staticWindowsTimerStuff()
    {
        timeEndPeriod( _timer_res );
    }

    UINT getTimerRes() const
    {
        return _timer_res;
    }

    void spinlockSleep( uint32_t usecs )
    {
        return; // As experiment, we will comment this on the next build and see what happens...
        /*        LARGE_INTEGER li;
        QueryPerformanceCounter( &li );
        int64_t start = li.QuadPart;
        int64_t end = start + (int64_t)((usecs) * _freq);
        while ( li.QuadPart < end )
            QueryPerformanceCounter( &li );*/
    }

    static void CALLBACK timerProc(UINT /*timer_id*/, UINT, DWORD ptr, DWORD, DWORD)
    {
        __staticWindowsTimerStuff_cbData* data = static_cast<__staticWindowsTimerStuff_cbData*>( (void*)ptr );
        // codice spostato fuori dalla callback
        //        if ( data->usWait > 0 )
        //            __staticWindowsTimerStuff::initialize()->spinlockSleep( data->usWait );
        data->mutex.unlock();
    }

private:
    __staticWindowsTimerStuff()
    {
        _timer_res = 100000;
        _freq = 100000.0;
        // init millisec timer
        TIMECAPS tc;
        if ( timeGetDevCaps( &tc, sizeof(TIMECAPS)) == TIMERR_NOERROR )
        {
            _timer_res = tc.wPeriodMin;
            timeBeginPeriod( _timer_res );
        }
        // init spinlock timer
        QueryPerformanceCounter( &li_count );
        QueryPerformanceFrequency( &li_freq );
        _freq = (double)(li_freq.QuadPart)/1000000.0;

        //        debugPrint( "WindowsTimers", DebugPrint::NOTICE_LEVEL ) << "Initiated Windows Timing with spinlock frequency of " << _freq << " and non-spinlock resolution of " << _timer_res << ". For better results you should switch to Linux.\n";
    }

    UINT _timer_res;
public:
    LARGE_INTEGER li_count;
    LARGE_INTEGER li_freq;
    double _freq;

};
#elif defined(FRAMEWORK_PLATFORM_LINUX)
#include <sys/time.h>
#endif

#include <time.h>

}

int64_t FrameworkTimer::__FrameworkTimerPrivate::usleep( uint64_t usecs )
{
    int64_t ret = 0;
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
    __staticWindowsTimerStuff* wt = __staticWindowsTimerStuff::initialize();
    UINT timer_res_ms = wt->getTimerRes();
    UINT timer_res_us = timer_res_ms * 1000;
    if ( usecs >= timer_res_us )
    {   // Over ms precision we can use the multimedia timers, which is nice to the CPU:
        __staticWindowsTimerStuff_cbData data;
        data.usWait = usecs % 1000;
        data.mutex.lock();
        UINT timer_id = timeSetEvent( usecs / timer_res_us, 0, (LPTIMECALLBACK)__staticWindowsTimerStuff::timerProc, (DWORD)(&data), TIME_ONESHOT | TIME_KILL_SYNCHRONOUS );
        data.mutex.lock();
        if ( data.usWait > 0 )
            __staticWindowsTimerStuff::initialize()->spinlockSleep( data.usWait );
        timeKillEvent( timer_id );
        data.mutex.unlock();
    }
    else
    {   // under ms we need precision, so we go spinlock...
        wt->spinlockSleep( usecs );
    }
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    struct timespec wait_for;
    wait_for.tv_sec = usecs / 1000000;
    wait_for.tv_nsec = (usecs * 1000ULL) % 1000000000ULL;
    struct timespec remaining;
    remaining.tv_sec = 0;
    remaining.tv_nsec = 0;
    ret = clock_nanosleep( CLOCK_MONOTONIC, 0, &wait_for, &remaining );
    if ( ret > 0 )
    {
        if ( ret == EINTR )
            ret = remaining.tv_sec * 1000000 + remaining.tv_nsec / 1000;
        else
            ret = -1;
    }
#endif
    return ret;
}

FrameworkTimer::FrameworkTimer(bool autostart)
{
    _private = new FrameworkTimer::__FrameworkTimerPrivate;
    _private->_loop_interval = 0;
    _private->_stop_time = _private->_start_time = 0;
    _private->_running = false;
    _private->_paused = false;
    _private->_loop_time = 0;
    _private->_quiet = false;
    if ( autostart )
        start();
}

FrameworkTimer::~FrameworkTimer()
{
    delete _private;
    _private = NULL;
}

TESTPOINT(LIB_6)
uint64_t FrameworkTimer::start()
{
    _private->_running = true;
    return _private->_stop_time = _private->_loop_time = _private->_start_time = getCurrentTime();
}

TESTPOINT(LIB_6)
bool FrameworkTimer::isRunning() const
{
    return _private->_running;
}

TESTPOINT(LIB_5)
bool FrameworkTimer::isPaused() const
{
    return _private->_paused;
}

TESTPOINT(LIB_6)
void FrameworkTimer::reset()
{
    stop();
    start();
}

TESTPOINT(LIB_6)
void FrameworkTimer::stopReset()
{
    stop();
    _private->_stop_time = _private->_loop_time = _private->_start_time = 0;
}

TESTPOINT(LIB_5)
void FrameworkTimer::pause()
{
    _private->_stop_time = getCurrentTime();
    _private->_paused = true;
}

TESTPOINT(LIB_5)
void FrameworkTimer::resume()
{
    _private->_paused = false;
}

TESTPOINT(LIB_4)
void FrameworkTimer::setLoopTime(uint64_t usecs)
{
    if ( !_private->_running )
        _private->_loop_interval = usecs;
}

TESTPOINT(LIB_4)
int64_t FrameworkTimer::waitLoop()
{

    if ( !_private->_running )
        start();

#if defined(FRAMEWORK_PLATFORM_WINDOWS)

    uint64_t elapsed = getCurrentTime() - _private->_loop_time;
    int64_t delta_time = elapsed - _private->_loop_interval;
    if ( delta_time < 0 )
        delta_time = -usleep( -delta_time );
    else if ( (delta_time > 0) && !_private->_quiet )
        debugPrint( "FrameworkTimer", DebugPrint::TIMER_CLASS ) << "WARNING: loop time overrun! (" << delta_time << "us over " << _private->_loop_interval << "us)\n";
    _private->_loop_time = getCurrentTime();
    return delta_time;

#elif defined(FRAMEWORK_PLATFORM_LINUX)

    uint64_t enter_time = getCurrentTime();
    int64_t elapsed_time = enter_time - _private->_loop_time;
    int64_t missing_delta = -1;
    if ( elapsed_time >= 0 )
    {
        missing_delta = _private->_loop_interval - elapsed_time;

        if ( missing_delta >= 0 )
        {
            _private->_loop_time += _private->_loop_interval;
            missing_delta = -usleep( missing_delta );
        }
        else // if ( missing_delta < 0 ) it's always true here
        {
            uint16_t n_cycles = (uint16_t)(elapsed_time / _private->_loop_interval);
            _private->_loop_time += ( n_cycles * _private->_loop_interval);
            missing_delta = -missing_delta;

            if ( !_private->_quiet )
                debugPrint( "FrameworkTimer", DebugPrint::TIMER_CLASS ) << "WARNING: loop time overrun! (" << missing_delta << "us over " <<
                                                                           _private->_loop_interval << "us) - lost " << n_cycles << " cycles" <<
                                                                           " elapsed_time=" << elapsed_time << "\n";
        }
    }
    else
        debugPrintError( "FrameworkTimer" ) << "ERROR: waitLoop jump back in the time!"
                                            << " - enter_time=" << enter_time
                                            << " - loop_time=" << _private->_loop_time
                                            << " - loop_interval=" << _private->_loop_interval << "\n";

    return missing_delta;

#endif
}

void FrameworkTimer::setQuiteMode(bool shutup)
{
    _private->_quiet = shutup;
}

TESTPOINT(LIB_4)
bool FrameworkTimer::elapsedLoop() const
{
    if ( _private->_running )
    {
        uint64_t elapsed = getCurrentTime() - _private->_loop_time;
        if ( elapsed > _private->_loop_interval )
            return true;
    }
    return false;
}

TESTPOINT(LIB_3)
uint64_t FrameworkTimer::elapsedTime() const
{
    return (
                (_private->_running && !_private->_paused) ?
                    getCurrentTime() : _private->_stop_time) - _private->_start_time;
}

TESTPOINT(LIB_3)
uint64_t FrameworkTimer::elapsedTimeMS() const
{
    return (
                (
                    (_private->_running && !_private->_paused) ?
                        getCurrentTime() : _private->_stop_time) - _private->_start_time )
            /1000ULL;
}

TESTPOINT(LIB_3)
uint64_t FrameworkTimer::elapsedTimeS() const
{
    return (
                (
                    (_private->_running && !_private->_paused) ?
                        getCurrentTime() : _private->_stop_time) - _private->_start_time )
            /1000000ULL;
}

TESTPOINT(LIB_6)
uint64_t FrameworkTimer::stop()
{
    _private->_stop_time = getCurrentTime();
    _private->_running = false;
    return elapsedTime();
}

TESTPOINT(LIB_2)
int64_t FrameworkTimer::usleep(int64_t usecs)
{
    return FrameworkTimer::__FrameworkTimerPrivate::usleep( usecs );
}

TESTPOINT(LIB_2)
int64_t FrameworkTimer::sleep(int32_t secs)
{
    return FrameworkTimer::__FrameworkTimerPrivate::usleep( secs * 1000000 );
}

TESTPOINT(LIB_2)
int64_t FrameworkTimer::msleep(int32_t msecs)
{
    return FrameworkTimer::__FrameworkTimerPrivate::usleep( msecs * 1000 );
}

TESTPOINT(LIB_2)
int64_t FrameworkTimer::usleep_s(int64_t usecs)
{
    return FrameworkTimer::__FrameworkTimerPrivate::usleep( usecs );
}

TESTPOINT(LIB_2)
int64_t FrameworkTimer::sleep_s(int32_t secs)
{
    return FrameworkTimer::__FrameworkTimerPrivate::usleep( secs * 1000000 );
}

TESTPOINT(LIB_2)
int64_t FrameworkTimer::msleep_s(int32_t msecs)
{
    return FrameworkTimer::__FrameworkTimerPrivate::usleep( msecs * 1000 );
}

TESTPOINT(LIB_1)
uint64_t FrameworkTimer::getCurrentTime()
{
#if defined(FRAMEWORK_PLATFORM_WINDOWS)
#if 0
    FILETIME time;
    GetSystemTimeAsFileTime( &time );
    return (((uint64_t)time.dwHighDateTime) << 32ULL | ((uint64_t)time.dwLowDateTime) ) / 10; // unita' 100nsecs -> divido per 10 per avere i usecs
#else
    __staticWindowsTimerStuff* wt = __staticWindowsTimerStuff::initialize();
    LARGE_INTEGER li;
    QueryPerformanceCounter( &li );
    return (uint64_t)(((li.QuadPart - wt->li_count.QuadPart) * 1000000ULL ) / wt->li_freq.QuadPart);
#endif
#elif defined(FRAMEWORK_PLATFORM_LINUX)
    struct timespec t;
    if ( clock_gettime( CLOCK_MONOTONIC, &t ) >= 0 )
        return (t.tv_sec * 1000000ULL + t.tv_nsec/1000ULL);
    debugPrintError( "FrameworkTimer" ) << "Error clock_gettime: (%d)" << errno << "\n";
    return 0;
#endif
}

TESTPOINT(LIB_1)
uint64_t FrameworkTimer::getCurrentTimeS()
{
    return getCurrentTime() / 1000000ULL;
}

TESTPOINT(LIB_1)
uint64_t FrameworkTimer::getTimeEpoc()
{
#if defined(FRAMEWORK_PLATFORM_WINDOWS) && !defined(FRAMEWORK_PLATFORM_WINDOWS_MINGW32)
    return _time64(NULL);
    //#elif defined(FRAMEWORK_PLATFORM_LINUX)
#else
    return ::time(NULL);
#endif
}
