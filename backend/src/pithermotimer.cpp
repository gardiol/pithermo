#include "pithermotimer.h"

#include <stdio.h>
#include <time.h>
#include <errno.h>

int64_t PithermoTimer::_usleep( uint64_t usecs )
{
    int64_t ret = 0;
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
    return ret;
}

PithermoTimer::PithermoTimer(bool autostart)
{
    _loop_interval = 0;
    _stop_time = _start_time = 0;
    _running = false;
    _paused = false;
    _loop_time = 0;
    _quiet = false;
    if ( autostart )
        start();
}

PithermoTimer::~PithermoTimer()
{
}

uint64_t PithermoTimer::start()
{
    _running = true;
    return _stop_time = _loop_time = _start_time = getCurrentTime();
}

bool PithermoTimer::isRunning() const
{
    return _running;
}

bool PithermoTimer::isPaused() const
{
    return _paused;
}

void PithermoTimer::reset()
{
    stop();
    start();
}

void PithermoTimer::stopReset()
{
    stop();
    _stop_time = _loop_time = _start_time = 0;
}

void PithermoTimer::pause()
{
    _stop_time = getCurrentTime();
    _paused = true;
}

void PithermoTimer::resume()
{
    _paused = false;
}

void PithermoTimer::setLoopTime(uint64_t usecs)
{
    if ( !_running )
        _loop_interval = usecs;
}

int64_t PithermoTimer::waitLoop()
{
    if ( !_running )
        start();

    uint64_t enter_time = getCurrentTime();
    int64_t elapsed_time = enter_time - _loop_time;
    int64_t missing_delta = -1;
    if ( elapsed_time >= 0 )
    {
        missing_delta = _loop_interval - elapsed_time;

        if ( missing_delta >= 0 )
        {
            _loop_time += _loop_interval;
            missing_delta = -usleep( missing_delta );
        }
        else // if ( missing_delta < 0 ) it's always true here
        {
            uint16_t n_cycles = (uint16_t)(elapsed_time / _loop_interval);
            _loop_time += ( n_cycles * _loop_interval);
            missing_delta = -missing_delta;
        }
    }
    return missing_delta;
}

void PithermoTimer::setQuiteMode(bool shutup)
{
    _quiet = shutup;
}

bool PithermoTimer::elapsedLoop() const
{
    if ( _running )
    {
        uint64_t elapsed = getCurrentTime() - _loop_time;
        if ( elapsed > _loop_interval )
            return true;
    }
    return false;
}

uint64_t PithermoTimer::elapsedTime() const
{
    return (
                (_running && !_paused) ?
                    getCurrentTime() : _stop_time) - _start_time;
}

uint64_t PithermoTimer::elapsedTimeMS() const
{
    return (
                (
                    (_running && !_paused) ?
                        getCurrentTime() : _stop_time) - _start_time )
            /1000ULL;
}

uint64_t PithermoTimer::elapsedTimeS() const
{
    return (
                (
                    (_running && !_paused) ?
                        getCurrentTime() : _stop_time) - _start_time )
            /1000000ULL;
}

uint64_t PithermoTimer::stop()
{
    _stop_time = getCurrentTime();
    _running = false;
    return elapsedTime();
}

int64_t PithermoTimer::usleep(int64_t usecs)
{
    return _usleep( usecs );
}

int64_t PithermoTimer::sleep(int32_t secs)
{
    return _usleep( secs * 1000000 );
}

int64_t PithermoTimer::msleep(int32_t msecs)
{
    return _usleep( msecs * 1000 );
}

int64_t PithermoTimer::usleep_s(int64_t usecs)
{
    return _usleep( usecs );
}

int64_t PithermoTimer::sleep_s(int32_t secs)
{
    return _usleep( secs * 1000000 );
}

int64_t PithermoTimer::msleep_s(int32_t msecs)
{
    return _usleep( msecs * 1000 );
}

uint64_t PithermoTimer::getCurrentTime()
{
    struct timespec t;
    if ( clock_gettime( CLOCK_MONOTONIC, &t ) >= 0 )
        return (t.tv_sec * 1000000ULL + t.tv_nsec/1000ULL);
    return 0;
}

uint64_t PithermoTimer::getCurrentTimeS()
{
    return getCurrentTime() / 1000000ULL;
}

uint64_t PithermoTimer::getTimeEpoc()
{
    return ::time(NULL);
}
