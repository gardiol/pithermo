#ifndef RUNNERTHREAD_H
#define RUNNERTHREAD_H

#include <list>

#include "basemutex.h"
#include "scheduledthread.h"
#include "sharedmemory.h"

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
    enum ModeType { MANUAL_MODE,
                    AUTO_MODE };

    RunnerThread(ConfigFile* config,
                  const std::string& hst,
                  Logger* l);
    virtual ~RunnerThread();

    void appendCommand( Command* cmd );

private:
    bool scheduledRun(uint64_t, uint64_t);
    bool _checkCommands();
    bool _checkAntiIce();
    bool _checkFlameout();
    bool _checkTargetTemperature(float sensor_temp,
                                 float target_temperature,
                                 bool &prev_over_temp,
                                 bool &prev_under_temp);
    bool _updateCurrentTime(uint64_t new_time);
    void _updateSmartTemp();
    void _updateStatus();
    void _saveConfig();

    SharedMemory _shared_status;
    Logger* _logger;
    Generator* _gas;
    Generator* _pellet;
    TempSensor* _temp_sensor;
    Program _program;
    History _history;

    ConfigFile* _config;

    std::list<Command*> _commands_list;
    BaseMutex _commands_mutex;

    ModeType _current_mode;
    bool _heating_activated;
    bool _smart_temp_on;

    // Special conditions:
    bool _anti_ice_active;
    bool _over_temp;
    bool _under_temp;
    bool _pellet_flameout;

    bool _prev_pellet_hot;
    float _min_temp;
    float _max_temp;
    float _smart_temp;
    float _temp_correction;

    uint64_t _sensor_success_reads;

    uint64_t _last_time;
    int32_t _day;
    int32_t _hour;
    int32_t _half_hour;

    float _current_ext_temp;
    float _current_ext_humidity;

    uint64_t _pellet_startup_delay;
    float _hysteresis;
};

#endif // RUNNERTHREAD_H
