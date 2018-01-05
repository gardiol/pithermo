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

    uint8_t fixReading(const int read);

    void saveConfig();
    void updateStatus();
    void updateHistory();

    void readSensor();
    void setGpioBool( uint8_t num, bool activate );
    bool readGpioBool( uint8_t num );

    std::vector<std::string> _status_json_template;
    std::string _config_file;
    std::string _exchange_path;
    std::string _history_file;

    std::list<Command*> _commands_list;
    BaseMutex _commands_mutex;

    bool _pellet_on;
    bool _pellet_minimum;
    bool _pellet_feedback;
    bool _gas_on;
    bool _manual_mode;
    float _current_humidity;
    float _current_temp;
    float _min_temp;
    float _max_temp;
    float _temp_correction;
    FrameworkTimer _sensor_timer;
    uint64_t _sensor_failed_reads;
    uint64_t _sensor_success_reads;

    uint8_t _pellet_command_gpio;
    uint8_t _pellet_minimum_gpio;
    uint8_t _pellet_feedback_gpio;
    uint8_t _gas_command_gpio;
    uint8_t _sensor_gpio;

    Program _program;

    bool _gpio_error;
    bool _history_warned;
    bool _print_sensor;
    uint32_t _last_time;

    std::list<float> _temp_history;
    std::string _str_manual;
    std::string _str_auto;
    std::string _str_on;
    std::string _str_off;
    std::string _str_min_t;
    std::string _str_max_t;
    std::string _str_day;
    std::string _str_h;
    std::string _str_f;
};

#endif // RUNNERTHREAD_H
