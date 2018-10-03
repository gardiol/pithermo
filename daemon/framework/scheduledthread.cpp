#include "scheduledthread.h"
#include "debugprint.h"

#include "memorychecker.h"
#include "frameworkutils.h"
#include "frameworktimer.h"

#include <stdio.h>

using namespace FrameworkLibrary;

namespace FrameworkLibrary {

class ScheduledThread::__ScheduledThreadPrivate
{
public:
    explicit __ScheduledThreadPrivate( const std::string& name):
        _stats_mutex( name + "stats_mutex" )
    {
        _freq_us = 0;
        _average_time_us = 0;
    }

    ~__ScheduledThreadPrivate()
    {
    }

    uint64_t _freq_us;
    uint64_t _average_time_us;
    BaseMutex _stats_mutex;
};

}

ScheduledThread::ScheduledThread(const std::string &name, uint64_t freq_us):
    BaseThread( name )
{
    _private = new __ScheduledThreadPrivate( name );
    _private->_freq_us = freq_us;
}

ScheduledThread::~ScheduledThread()
{
    requestTerminate();
    waitForEnd();
    delete _private;
    _private = nullptr;
}

void ScheduledThread::setFrequency(uint64_t freq_us)
{
    if ( !isRunning() )
    {
        _private->_freq_us = freq_us;
        debugPrint( getName(), DebugPrint::THREAD_CLASS ) << "SCHEDULED frequency set at " << freq_us << "us\n";
    }
    else
        debugPrintError( getName() ) << "Unable to set frequency if thread is already running!\n";
}

bool ScheduledThread::scheduleStart()
{
    return true;
}

void ScheduledThread::schedulingStopped()
{

}

double ScheduledThread::getLastSecondLoad() const
{
    double ret = 0.0;
    _private->_stats_mutex.lock();
    if ( _private->_freq_us > 0 )
        ret = (double)_private->_average_time_us / (double)_private->_freq_us;
    _private->_stats_mutex.unlock();
    return ret;
}

uint64_t ScheduledThread::getLastSecondTime() const
{
    uint64_t ret = 0;
    _private->_stats_mutex.lock();
    ret = _private->_average_time_us;
    _private->_stats_mutex.unlock();
    return ret;
}

void ScheduledThread::run()
{
    if ( _private->_freq_us > 0 )
    {
        _private->_average_time_us = 0;
        std::list<uint64_t> average_time_pool;
        uint64_t total_time_last_sec = 0;
        uint64_t n_samples = FrameworkUtils_max<uint64_t>( 1, 1000000 / _private->_freq_us );

        if ( scheduleStart() )
        {
            debugPrint( getName(), DebugPrint::THREAD_CLASS ) << "Starting SCHEDULED thread at " << _private->_freq_us << "us\n";
            FrameworkTimer run_timer;
            bool run_ok = true;
            uint64_t cycle = 0;
            run_timer.setLoopTime( _private->_freq_us );
            while ( !terminationRequested() && run_ok )
            {
                FrameworkTimer elapsed_timer( true );
                run_ok = scheduledRun( run_timer.elapsedTime() , cycle++ );
                uint64_t elapsed_time = elapsed_timer.stop();

                // Update last second average:
                total_time_last_sec += elapsed_time;
                average_time_pool.push_back( elapsed_time );
                uint64_t store_samples = average_time_pool.size();
                if ( store_samples > n_samples )
                {
                    uint64_t to_be_removed = average_time_pool.front();
                    average_time_pool.pop_front();
                    total_time_last_sec -= to_be_removed;
                }
                _private->_stats_mutex.lock();
                _private->_average_time_us =  total_time_last_sec / store_samples;
                _private->_stats_mutex.unlock();

                run_timer.waitLoop();
            }
            schedulingStopped();
            debugPrint( getName(), DebugPrint::THREAD_CLASS ) << "Stopped SCHEDULED thread.\n";
            _private->_average_time_us = 0;
        }
        else
            debugPrint( getName(), DebugPrint::THREAD_CLASS ) << "Unable to start thread due to scheduleStart() return false\n";
    }
    else
        debugPrintError( getName() ) << "ERROR! Scheduled thread cannot be started with a frequency of 0us!\n";
}

void ScheduledThread::customPrintStatistics()
{
    if ( _private->_freq_us > 0 )
        debugPrintUntagged() << "     -> scheduled at " << FrameworkUtils::human_readable_number( _private->_freq_us, "us", false, FrameworkUtils::DECIMAL_TYPE, 0) <<
                           ". Average time: " << FrameworkUtils::human_readable_number( getLastSecondTime(), "us", false, FrameworkUtils::DECIMAL_TYPE, 0) <<
                           " / Load: " << (getLastSecondLoad() * 100.0) <<
                           "%\n";
    else
        debugPrintUntagged() << "     -> not yet scheduled.\n";
}
