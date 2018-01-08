#ifndef RUNNERTHREAD_H
#define RUNNERTHREAD_H

#include <list>

#include "basemutex.h"
#include "scheduledthread.h"

#include "program.h"
#include "historyitem.h"
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

    void saveConfig();
    void updateStatus(bool gas_on, bool pellet_on, bool pellet_minimum, bool pellet_feedback);
    void updateHistory(float last_temp, float last_humidity, uint32_t last_time);
    void writeHistoryJson();

    bool readSensor(float &current_temp, float &current_humidity);
    void setGpioBool( uint8_t num, bool activate );
    bool readGpioBool( uint8_t num );

    std::vector<std::string> _status_json_template;
    std::string _config_file;
    std::string _exchange_path;
    std::string _history_file;

    std::list<Command*> _commands_list;
    BaseMutex _commands_mutex;

    bool _manual_mode;
    bool _anti_ice_active;
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
    float _current_humidity;

    Program _program;

    bool _gpio_error;
    bool _history_warned;

    std::list<HistoryItem> _th_history;
    std::string _str_manual;
    std::string _str_auto;
    std::string _str_on;
    std::string _str_off;
    std::string _str_min_t;
    std::string _str_max_t;
    std::string _str_day;
    std::string _str_h;
    std::string _str_f;
    uint32_t _day;
    uint32_t _hour;
    uint32_t _half_hour;

    static const uint32_t default_history_items = 100;
};

#endif // RUNNERTHREAD_H
