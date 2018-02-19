#ifndef RUNNERTHREAD_H
#define RUNNERTHREAD_H

#include <list>

#include "basemutex.h"
#include "scheduledthread.h"

#include "program.h"
#include "history.h"
#include "logger.h"
#include "generator.h"
#include "tempsensor.h"

using namespace FrameworkLibrary;

class Command;

class RunnerThread : public ScheduledThread
{
public:
    RunnerThread( const std::string& cfg,
                  const std::string& exchange_path,
                  const std::string& hst,
                  Logger* l);
    virtual ~RunnerThread();

    void appendCommand( Command* cmd );

private:
    bool scheduledRun(uint64_t, uint64_t);
    void _updateCurrentTime(uint64_t new_time);
    void _updateStatus();
    bool _checkCommands();
    bool _checkFlameout();
    bool _checkSpecialConditions();
    void _saveConfig();

    Logger* _logger;
    Generator* _gas;
    Generator* _pellet;
    TempSensor* _temp_sensor;
    Program _program;
    History _history;

    std::vector<std::string> _status_json_template;
    std::string _config_file;
    std::string _exchange_path;

    std::list<Command*> _commands_list;
    BaseMutex _commands_mutex;

    bool _manual_mode;
    bool _manual_gas_on;

    // Special conditions:
    bool _anti_ice_active;
    bool _over_temp;
    bool _under_temp;
    bool _pellet_flameout;

    bool _prev_pellet_hot;
    float _min_temp;
    float _max_temp;
    float _temp_correction;

    uint64_t _sensor_success_reads;

    uint64_t _last_time;
    int32_t _day;
    int32_t _hour;
    int32_t _half_hour;

    float _current_ext_temp;

    uint64_t _pellet_startup_delay;
};

#endif // RUNNERTHREAD_H
