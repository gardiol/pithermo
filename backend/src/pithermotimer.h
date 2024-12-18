#ifndef PITHERMOTIMER_H
#define PITHERMOTIMER_H

#include <stdint.h>

class PithermoTimer
{
public:
    PithermoTimer(bool autostart = false);
    virtual ~PithermoTimer();

    uint64_t start();
    bool isRunning() const;    
    bool isPaused() const;
    void reset();
    void stopReset();
    void pause();
    void resume();
    void setLoopTime( uint64_t usecs );
    int64_t waitLoop();
    void setQuiteMode( bool shutup );
    bool elapsedLoop() const;
    uint64_t elapsedTime() const;
    uint64_t elapsedTimeMS() const;
    uint64_t elapsedTimeS() const;
    uint64_t stop();
    int64_t usleep( int64_t usecs );
    int64_t sleep( int32_t secs );
    int64_t msleep(int32_t msecs );

    static int64_t usleep_s( int64_t usecs );
    static int64_t sleep_s( int32_t secs );
    static int64_t msleep_s(int32_t msecs );
    static uint64_t getCurrentTime();
    static uint64_t getCurrentTimeS();
    static uint64_t getTimeEpoc();

private:
    static int64_t _usleep( uint64_t usecs );

    uint64_t _start_time;
    uint64_t _stop_time;
    uint64_t _loop_time;
    uint64_t _loop_interval;
    bool _quiet;
    bool _running;
    bool _paused;
};

#endif // PITHERMOTIMER_H
