#ifndef TEMPSENSOR_H
#define TEMPSENSOR_H

#include <stdint.h>

#include "gpiodevice.h"
#include "logger.h"

#include "frameworktimer.h"

using namespace FrameworkLibrary;

class TempSensor: protected GPIODevice
{
public:
    TempSensor(Logger* l, uint8_t gpio, float temp_correction );
    virtual ~TempSensor();

    bool readSensor();

    float getTemp() const
    {
        return _temp;
    }

    float getHimidity() const
    {
        return _humidity;
    }

    inline uint64_t getTimestamp() const
    {
        return _timestamp;
    }

    void printStatus();

private:
    Logger* _logger;
    float _temp;
    float _humidity;
    uint64_t _timestamp;
    FrameworkTimer _timer;
    uint8_t _gpio;
    float _temp_correction;
    bool _at_lease_one_read_ok;
};

#endif // TEMPSENSOR_H
