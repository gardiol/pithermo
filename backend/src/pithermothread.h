#ifndef PITHERMOTHREAD_H
#define PITHERMOTHREAD_H

#include <stdint.h>

#include <string>

typedef uint32_t BaseThreadId;

class PithermoMutex;

class PithermoThread
{
    friend class __PithermoThreadPrivate;
public:
    PithermoThread( const std::string& name, uint64_t freq_us = 0 );
    virtual ~PithermoThread();

    void setName( const std::string& name );
    static BaseThreadId getThreadId();
    bool isRunning() const;
    std::string getName() const;
    int waitForEnd();
    void requestTerminate();
    void startThread();
    bool terminationRequested()  const;
    void setFrequency( uint64_t freq_us );

protected:
    virtual void run();
    void setReturnValue(int val );
    virtual bool scheduledRun( uint64_t elapsed_time_us, uint64_t cycle) = 0;
    virtual bool scheduleStart();
    virtual void schedulingStopped();

private:
    class __PithermoThreadPrivate;
    __PithermoThreadPrivate* _private;

};

#endif // PITHERMOTHREAD_H
