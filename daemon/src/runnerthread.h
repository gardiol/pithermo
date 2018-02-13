#ifndef RUNNERTHREAD_H
#define RUNNERTHREAD_H

#include <list>

#include "basemutex.h"
#include "scheduledthread.h"

#include "program.h"
#include "history.h"
#include "logger.h"

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
    Logger* _logger;
    bool checkGas();
    void gasOn();
    void gasOff();
    bool checkPellet();
    void pelletOn();
    void pelletOff();
    bool checkPelletMinimum();
    void pelletMinimum( bool m );
    bool pelletFeedback();

    bool scheduledRun( uint64_t elapsed_time_us, uint64_t cycle);
    bool scheduleStart();
    void schedulingStopped()
    {
    }

    uint8_t fixReading(const int read);
    void updateCurrentTime();
    void updateProgram();
    void updateEventsJson(const std::list<LogItem> *events);
    void appendMessage( const std::string& msg );

    void saveConfig();
    void updateStatus();

    bool readSensor(float &current_temp, float &current_humidity);
    void setGpioBool( uint8_t num, bool activate );
    bool readGpioBool(uint8_t num , int mode);

    std::vector<std::string> _status_json_template;
    std::string _config_file;
    std::string _exchange_path;

    std::list<Command*> _commands_list;
    BaseMutex _commands_mutex;

    bool _manual_mode;
    bool _anti_ice_active;
    bool _over_temp;
    bool _under_temp;
    bool _gas_was_on_before_over;
    bool _pellet_flameout;
    bool _prev_pellet_feedback;
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

    uint64_t _last_time;
    float _current_temp;
    float _current_ext_temp;
    float _current_humidity;

    Program _program;
    bool _program_gas;
    bool _program_pellet;
    bool _program_pellet_minimum;

    bool _gpio_error;

    History _history;
    std::string _str_manual;
    std::string _str_auto;
    std::string _str_on;
    std::string _str_off;
    std::string _str_min_t;
    std::string _str_max_t;
    std::string _str_day;
    std::string _str_h;
    std::string _str_f;
    std::string _str_pellet_off_warning;
    std::string _str_pellet_on_warning;
    int32_t _day;
    int32_t _hour;
    int32_t _half_hour;
    std::string _str_temp;
    std::string _str_ext_temp;
    std::string _str_humidity;
    uint64_t _last_pellet_on_time;
    uint64_t _last_pellet_on_min_time;
    uint64_t _last_gas_on_time;
    uint64_t _today_pellet_time;
    uint64_t _today_pellet_min_time;
    uint64_t _today_gas_time;
    uint64_t _pellet_startup_delay;
};

#endif // RUNNERTHREAD_H
