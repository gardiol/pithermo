#ifndef BASETHREAD_H
#define BASETHREAD_H

#include <common_defs.h>

#include <string>
#include <map>

#include "frameworktimer.h"

namespace FrameworkLibrary {

#if defined(FRAMEWORK_PLATFORM_WINDOWS)
typedef DWORD BaseThreadId;
#elif defined(FRAMEWORK_PLATFORM_LINUX)
typedef uint32_t BaseThreadId;
#endif

class BaseMutex;
class __BaseThreadPrivate;

/**
  * @brief Implements threads in a multi-platform way
  *
  * You must derive a child class from BaseThread. This derived class must implement the run() pure virtual
  * method, which will be the thread function.
  *
  * # Thread Control #
  * To start the thread, call the BaseThread::start() method: the thread will be started immediately and the
  * BaseThread::run() will be executed in the new thread. To stop the thread execution you should call the
  * BaseThread::requestTerminate() method from outside the thread, while the BaseThread::run() method shall
  * periodically check the BaseThread::terminationRequested() and terminate it's execution adeguately.
  *
  * @warning there is no way to "force" a thread to terminate. This kind of operation is dangerous and
  * a bad practice, which is then not supported. For this reason it is of paramount importance that the code
  * executed in the BaseThread::run() method do properly return without hanging.
  *
  * You can check the thread execution by calling BaseThread::isRunning() and you can wait for the thread termination
  * by calling the blocking BaseThread::waitForEnd(), which will return only after the thread has terminated,
  * or will hang indefinitely otherwise.
  *
  * Upon class delete, the thread will be terminated by calling BaseThread::requestTerminate() and
  * BaseThread::waitForEnd(), so it will HANG unless the thread behaves properly.
  *
  * # timer #
  * Using BaseThread::elapsedTime() you can know for how many milliseconds the thread has been running.
  *
  * If all you need is a periodic thread, something like:
  * @code
  * while ( ! terminate )
  * {
  *      ... do your stuff ...
  *      sleep( some amount of time );
  * }
  * @endcode
  *
  * Then check out the ScheduledThread class, which might just be what you need!
  *
  * You can print on screen threads statistics at any time by calling BaseThread::printStatistics().
  *
  * You can also get the ID of the current thread by calling the static BaseThread::getThreadId().
  *
  * @include example_cpp_multithread.cpp
  * @include example_cpp_semaphor.cpp

  *
  */
class DLLEXPORT BaseThread
{
    friend class __BaseThreadPrivate;
public:
    /** @brief print statistics on all the threads
     */
    static void printStatistics();

    /** @brief Create a thread
     * @param name name of thread
    */
    BaseThread( const std::string& name );
    virtual ~BaseThread();

    /** @brief Set the thread name.
     *
     * Change or set thread name
     * @param name new theead name
     */
    void setName( const std::string& name );

    /** @brief Return the thread-id
     *
     * @warning this returns the thread-id of the caller thread, NOT an ID of an instance of the class.
     * This is the reason why this method is static and not a class memeber.
     * @return thread-id of the caller thread.
     * 
     */
    static BaseThreadId getThreadId();

    /** @brief return true if the thread is running
      * @return true if the thread is running, false otherwise.
     * 
      */
    bool isRunning() const;

    /** @brief Get thread name
     * @return thread name
     * @since 4.0.2
     */
    std::string getName() const;

    /** @brief Wait until the thread terminates (blocking, no timeout)
     *
     *  Wait for the thread to terminate. If it is not in execution will return immediately with
     * the last known return value (-1 if it has never run).
     * If the thread is running, will block until the run() method has terminated. Be careful in your run()
     * method, check the request_terminate attribute and make sure you return!
      * @return the thread return value. Use isRunning() to make sure it terminated!
     * 
      */
    int waitForEnd();

    /** @brief Request the thread to terminate. Will return immediately.
     * 
      */
    void requestTerminate();

    /** @brief Current thread execution time, in ms
      * @return milliseconds since thread start
      * @since 3.3
     * 
      */
    uint64_t elapsedTimeMs() const;

    /** @since 3.3
     * @brief Start the thread (if already running, it has no effect)
     * 
      */
    void startThread();

    /** @brief Check if thread termination has been requested or not
     * @return true if the thread should terminate, false otherwise.
     * 
     */
    bool terminationRequested()  const;

protected:
    /** @brief Main thread function
     *
     * This method must be derived in child classes. This is the method executed in the new thread.
      * *WARNING:* if the run() method does not return according to the terminate_thread attribute, then
      * it might block any other thread calling waitForEnd() or, since it's called in the destructor,
      * the entire program proper shutdown.
      *
      * If you just need to run scheduled at some frequency (if all you need is a while...sleep approach)
      * see the ScheduledThread class, which might just be what you need.
     * 
      */
    virtual void run() = 0;

    /** @brief call this method to set the return value
     *
     * @warning you MUST call this to set a proper return value! Any value set by the operating system methods will not work.
     * @param val return value for the thread
     * 
     */
    void setReturnValue(int val );

    /** @brief Implement this if you need to add your own statistics printouts
     */
    virtual void customPrintStatistics();

private:
    __BaseThreadPrivate* _pdata;
};

}

#endif // BASETHREAD_H
