#ifndef RUNNERTHREAD_H
#define RUNNERTHREAD_H

#include "scheduledthread.h"

using namespace FrameworkLibrary;

class RunnerThread : public ScheduledThread
{
public:
    RunnerThread();
    virtual ~RunnerThread();

private:

    bool scheduledRun( uint64_t elapsed_time_us, uint64_t cycle);
    bool scheduleStart();
    void schedulingStopped();

};

#endif // RUNNERTHREAD_H
