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

#include "mqtt_interface.h"

using namespace FrameworkLibrary;

class Command;

class RunnerThread : public ScheduledThread, public MQTT_callback
{
public:
    enum ModeType { MANUAL_MODE = 1,
                    AUTO_MODE = 2,
                    EXTERNAL_MODE = 3
    };

    RunnerThread(ConfigFile* config,
                 const std::string& hst,
                 Logger* l);
    virtual ~RunnerThread();

    void appendCommand( Command* cmd );

private:
    void message_received( const std::string& topic, const std::string& payload);

    bool scheduledRun(uint64_t, uint64_t);
    bool _checkCommands();
    bool _checkAntiIce();
    bool _checkFlameout();
    bool _checkTargetTemperature(float sensor_temp,
                                 float target_temperature_max,
                                 float hysteresis_max,
                                 float target_temperature_min,
                                 float hysteresis_min,
                                 bool &prev_over_temp,
                                 bool &prev_under_temp);
    bool _checkExcessTemperature(float sensor_temp,
                                 float target_temperature,
                                 float excessive_overtemp_threshold,
                                 bool& excessive_overtemp );
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
    bool _excessive_overtemp;
    bool _gas_status_at_overtemp;
    bool _pellet_minimum_status_at_overtemp;

    bool _prev_pellet_hot;
    float _min_temp;
    float _max_temp;
    float _smart_temp;
    float _temp_correction;
    float _excessive_overtemp_threshold;

    bool _external_request;
    std::string _external_request_topic;

    uint64_t _sensor_success_reads;

    uint64_t _manual_off_time;
    uint64_t _last_time;
    int32_t _day;
    int32_t _hour;
    int32_t _half_hour;

    float _current_ext_temp;
    float _current_ext_humidity;

    uint64_t _pellet_startup_delay;
    float _hysteresis_max;
    float _hysteresis_min;

    bool _manual_pellet_minimum_forced_off;
    bool _manual_gas_forced_on;

    MQTT_Interface* _mqtt;
};

#endif // RUNNERTHREAD_H
