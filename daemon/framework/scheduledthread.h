#ifndef SCHEDULEDTHREAD_H
#define SCHEDULEDTHREAD_H

#include "common_defs.h"
#include "basethread.h"

namespace FrameworkLibrary {

/**
  * @brief Implements a thread which is scheduled automatically at a given frequency
  *
  * @todo add a C interface for this class
  *
  * This class implements a "scheduled" thread, which means a thread which is automatically
  * called at a given frequency.
  *
  * The virtual method scheduleStart() will be called once as soon as the thread is started, the schedulingStopped()
  * is called once just before the thread is stopped. In between, the scheduledRun() is called at the frequency
  * you have set.
  *
  * While the thread is stopped you can change the frequency. It has no effect while the thread is running.
  *
  * You can abort the thread from starting by returning false in scheduleStart(), while you can force stop the
  * thread by returning false at any time in scheduledRun().
  *
  * The thread will terminate automatically if requestTerminate() is called, without any need of code from your side.
  *
  * @include example_cpp_scheduledthread.cpp
  *
  */
class DLLEXPORT ScheduledThread:
        public BaseThread
{
    friend class __BaseThreadPrivate;
public:
    /** @brief Create a thread
     * @param freq_us thread loop frequency (0 = disabled by default)
     * @param name name of thread
    */
    ScheduledThread( const std::string& name, uint64_t freq_us = 0 );
    virtual ~ScheduledThread();

    /** @brief set a frequency.
     *
     * This method should be called only while the thread is not running.
     * If called while the thread is running, it will have no effect.
     * Set 0 to disable this thread operations.
     *
     * @param freq_us thread loop frequency.
     */
    void setFrequency( uint64_t freq_us );

    /** @brief return avergae load of last second
     * @note statistics are valid only for running threads
     * 0.0 means unloaded, 1.0 means fully loaded, more than 1.0 means overrunning.
     * @return load
     */
    double getLastSecondLoad() const;

    /** @brief return avergae execution time of last second
     * @note statistics are valid only for running threads
     * @return execution time in microseconds
     */
    uint64_t getLastSecondTime() const;

protected:
    /** @brief Periodically called run
     *
     * Implement here your code. This will be called at the frequency requested.
     * @warning If you run longer than the frequency requested, you will cause an overrun.
     * @param elapsed_time_us time, in microseconds, since thread has been started (this can overflow)
     * @param cycle number of cycles this method has been called since thread start (this can overflow)
     * @return Return true to keep running or false to stop execution.
     */
    virtual bool scheduledRun( uint64_t elapsed_time_us, uint64_t cycle) = 0;

    /** @brief do your pre-run checks and code here
     *
     * This is called once immediately before scheduling is started. Put your once-per-run initialization here.
     * @return true if all is ok and the scheduling can start
     */
    virtual bool scheduleStart();

    /** @brief do your post-run cheanup and code here
     *
     * This is called immediately after scheduling has terminated. Put your once-per-run shutdown here.
     */
    virtual void schedulingStopped();

private:
    void run();
    void customPrintStatistics();

private:
    class __ScheduledThreadPrivate;
    __ScheduledThreadPrivate* _private;
};

}

#endif // SCHEDULEDTHREAD_H
