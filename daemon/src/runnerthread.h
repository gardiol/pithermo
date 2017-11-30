#ifndef RUNNERTHREAD_H
#define RUNNERTHREAD_H

#include <list>

#include "basemutex.h"
#include "scheduledthread.h"

#include "program.h"

using namespace FrameworkLibrary;

class Command;

class RunnerThread : public ScheduledThread
{
public:
    RunnerThread( const std::string& cfg,
                  const std::string& exchange_path,
                  const std::string& hst);
    virtual ~RunnerThread();

    void appendCommand( Command* cmd );

private:
    bool scheduledRun( uint64_t elapsed_time_us, uint64_t cycle);
    bool scheduleStart();
    void schedulingStopped();

    void saveConfig();
    void updateStatus();
    void updateHistory();

    std::string _config_file;
    std::string _exchange_path;
    std::string _history_file;

    std::list<Command*> _commands_list;
    BaseMutex _commands_mutex;

    bool _pellet_command;
    bool _gas_command;
    bool _manual_mode;
    float _current_temp;
    float _min_temp;
    float _max_temp;

    bool _error;

    uint32_t _last_time;

    Program _program;

    bool _history_warned;

    std::list<float> _temp_history;
};

#endif // RUNNERTHREAD_H
