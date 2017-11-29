#ifndef FRAMEWORKTIMER_H
#define FRAMEWORKTIMER_H

#include <common_defs.h>

namespace FrameworkLibrary {

/*class DLLEXPORT FrameworkTimerCallback
{
public:
    virtual ~FrameworkTimerCallback();

protected:
    virtual void customTimerExpired( uint64_t interval ) = 0;

};*/

/** @brief Provide a microsecond precise timer, platform independent.
  *
  * This class provide timing, timestamp and synchronization functions.
  *
  * The FrameworkTimer can be used in three different modes:
  * - Simple counter: call FrameworkTimer::start() to start the timer and FrameworkTimer::stop() to stop it.
  * You can call FrameworkTimer::elapsedTime() to know how much time has elapsed. If the timer has been started but not stopped,
  * this is the time since to start to now, if the timer has been started and stopped this is the time between start and stop.
  *
  * - Simple wait: call rameworkTimer::usleep() (and similar methods) to sleep for the given time.
  *
  * - Precise wait: set a "loop time" with FrameworkTimer::setLoopTime(), then start the timer with FrameworkTimer::start().
  * You can know if the time you set has elapsed by calling FrameworkTimer::elapsedLoop(), which is non-blocking, or just wait out
  * all the remaining time by calling FrameworkTimer::waitLoop(), which will block until all the remaining time is consumed.
  *
  * @section FrameworkTimer_accuracy Notes on accuracy, drift and precision
  *
  * While the basic time unit is microsecond, actual precision will vary depending on the underlying operating system and
  * available hardware. Care has been taken to ensure the maximum possible precision using the available standard
  * APIs and libraries available on each supported platform, but no custom or commercial interfaces are supported
  * at this time.
  *
  * @subsection FrameworkTimer_accuracy_linux Specific Linux notes
  *
  * On Linux, without any specific real-time kernel or hardware, the following measurements have been observed:
  * - sleep accuracy: under 100us
  * - getCurrentTime() precision: on average 50% (1 each 2us), worst case 1,54% (1 each 64us)
  *
  * Which means that the getCurrentTime() get updated on average each 2us with a worst case of 64us.
  *
  * Since the "real-time" library is used internally, if your kernel and/or hardware does support real-time better results
  * are to be expected.
  *
  * @subsection FrameworkTimer_accuracy_win32 Specific Windows notes
  *
  * Windows is not anywhere close real-time and it's kernel is intrinsecally unable to keep up with anything remotely precise.
  * The basic timestamp is very imprecise and suffers from the internal "tick" of Windows which is around 15ms (15000us).
  * This holds true for all versions of Windows, from Windows XP to Windows 10 at the time of writing this document.
  *
  * Using specific advanced APIs and a mix of spinlocks, this class can achieve a much better precision. Still, it's much
  * worse than Linux and it cannot be improved due to the nature of Windows, unless specific hardware or commercial libraries
  * are used.
  *
  * The following measurements have been observed:
  * - sleep accuracy: between 690us and 2600us
  * - getCurrentTime() precision: on average 0.099% (1 each 1000us), worst case 0.049% (1 each 2000us)
  *
  * Which means that the getCurrentTime() get updated every 1ms or with a worst case of 2ms.
  *
  * In order to get any precision under 1ms, a spinlock mechanism is used under Windows. It will yeld less tha 10us precise
  * timing, but it will use up to 100% of CPU. Please be advise and use intervals and sleeps under 1ms sparringly on Windows.
  *
  * Example:
  * @include example_cpp_timer.cpp
  *
  *
  */
class DLLEXPORT FrameworkTimer
{
    public:
    /**
      * @param autostart if true, the timer is started immediately, otherwise call start()
      *
      *
      */
    FrameworkTimer(bool autostart = false);
    virtual ~FrameworkTimer();

    /** @brief Start the timer and return start time in microseconds
     * @return start time, in microseconds
      */
    uint64_t start();

    /** @brief Check if the timer is running
     * @since 3.3
     * @return true if the timer is started, false otherwise.
      *
     */
    bool isRunning() const;

    /** @brief Check if the timer is paused
     * @since 4.0.3
     * @return true if the timer is paused
     */
    bool isPaused() const;

    /** @brief stop and start (reset) a running timer, start a non-running timer.
     * @since 3.3
      *
     */
    void reset();

    /** @brief stop and zeroize (reset) a running timer, does not affect non-running timer.
     * @since 4.0.2
      *
     */
    void stopReset();

    /** @brief Pause the timer
     * @note This has no effect on the "wait loop"
     * @since 4.0.3
     */
    void pause();

    /** @brief Resume the timer
     * @note This has no effect on the "wait loop"
     * @since 4.0.3
     */
    void resume();

    /** @brief Set length of loop in microseconds
      *
      * See waitLoop() for more detauls.
      * @param usecs how many microseconds the loop should last
      *
      */
    void setLoopTime( uint64_t usecs );

    /** @brief Wait the required time set by setLoopTime().
      *
      * Initialize the loop time with setLoopTime(), then start the timer with start(). If you don't call start(), it will be called
      * automatically inside waitLoop() at the first iteration.
      *
      * The return value will tell you the status:
      * - If = 0: all was ok.
      * - If < 0: an error occurred, the returned value is the number of microsceonds still to be waited for (with minus sign)
      * - If > 0: the loop has already been breached. Ths number is by how many microseconds the loop has been superated.
      *
      * @return Microseconds since the end of the loop: 0 if all is ok, < 0 if the wait terminated early for some error, > 0 if the loop has been breached.
      *
      */
    int64_t waitLoop();

    /** @brief Make the timer less verbose in case of errors
     *
     * By default, quiet mode is disabled. Errors and overruns will be printed.
     * @since 4.0.2
     * @param shutup if true, no error messages will be printed.
     */
    void setQuiteMode( bool shutup );

    /** @brief Check if the loop timer set by setLoopTime() has elapsed.
     * @since 3.4
     *
      * Initialize the loop time with setLoopTime(), then start the timer with start().
      * You must start it, or this will always return false.
      * This is not blocking.
      *
      * @return true if the loop has elapsed, false if not.
      *
     */
    bool elapsedLoop() const;

    /** @brief Return how many microseconds have passed since start
      * @return microseconds
      *
      */
    uint64_t elapsedTime() const;

    /** @brief Return how many milliseconds have passed since start
      * @return milliseconds
      *
      */
    uint64_t elapsedTimeMS() const;

    /** @brief Return how many seconds have passed since start
      * @return seconds
      * @since 3.4
      *
      */
    uint64_t elapsedTimeS() const;

    /** @brief Stop the times
      * @return microseconds elapsed since start
      *
      */
    uint64_t stop();

    /**  @brief Sleeps for a given number of microsceonds
      * @param usecs how much time to wait for
      * @return how many microseconds not waited for (due to signal received), -1 if error
      *
      */
    int64_t usleep( int64_t usecs );

    /**  @brief Sleeps for a given number of seconds
      * @param secs how much time to wait for
      * @return how many microseconds not waited for (due to signal received), -1 if error
      *
      */
    int64_t sleep( int32_t secs );

    /**  @brief Sleeps for a given number of milliseconds
      * @param msecs how much time to wait for
      * @return how many microseconds not waited for (due to signal received), -1 if error
      *
      */
    int64_t msleep(int32_t msecs );

    /**  @brief Sleeps for a given number of microsceonds (static)
     * @since 3.4
      * @param usecs how much time to wait for
      * @return how many microseconds not waited for (due to signal received), -1 if error
      *
      */
    static int64_t usleep_s( int64_t usecs );

    /**  @brief Sleeps for a given number of seconds (static)
     * @since 3.4
      * @param secs how much time to wait for
      * @return how many microseconds not waited for (due to signal received), -1 if error
      *
      */
    static int64_t sleep_s( int32_t secs );

    /**  @brief Sleeps for a given number of milliseconds (static)
     * @since 3.4
      * @param msecs how much time to wait for
      * @return how many microseconds not waited for (due to signal received), -1 if error
      *
      */
    static int64_t msleep_s(int32_t msecs );

    /** @brief Current time, in microseconds, since system startup (arbitrary value, OS dependent)
     *
     * This is always monotonic.
     * @return microseconds
      *
     */
    static uint64_t getCurrentTime();

    /** @brief Current time, in seconds, since system startup (arbitrary value, OS dependent)
     *
     * This is always monotonic.
     * @return seconds
      *
     */
    static uint64_t getCurrentTimeS();

    /** @brief Return current time, in SECONDS, since UNIX EPOC (on all OS)
     * @return Seconds since EPOC (UNIX)
      *
     */
    static uint64_t getTimeEpoc();

    private:
    class __FrameworkTimerPrivate;
    __FrameworkTimerPrivate* _private;
};

}

#endif // FRAMEWORKTIMER_H
