#include "tempsensor.h"

#include "frameworkutils.h"

#include <math.h>

TempSensor::TempSensor(Logger *l, int gpio, float temp_correction):
    _logger(l),
    _temp(0.0),
    _humidity(0.0),
    _timestamp(0),
    _gpio(gpio),
    _temp_correction(temp_correction)
{
    _timer.setLoopTime( 8000 * 1000 );
}

TempSensor::~TempSensor()
{
}

bool TempSensor::readSensor()
{
    bool ret = false;
    if ( !_timer.isRunning() || _timer.elapsedLoop() )
    {
        ret = _rawRead();
        _timer.reset();
    }
    return ret;
}


bool TempSensor::_rawRead()
{
    bool ret = false;
#ifdef DEMO
    ret = true;
    _timestamp = FrameworkTimer::getTimeEpoc();
    _temp = (_timestamp/60) % 20 + 10;
    _humidity = (_timestamp/3600) % 70 + 25;
#else
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    int dht22_dat[5] = {0,0,0,0,0};
    uint8_t j = 0, i;

    // pull pin down for 18 milliseconds
    setGPIOoutput( _gpio );
    writeGPIObool( _gpio, false );
    FrameworkTimer::msleep_s( 18 );
    // then pull it up for 40 microseconds
    writeGPIObool( _gpio, true );
    FrameworkTimer::usleep_s( 40 );
    // prepare to read the pin
    setGPIOinput(_gpio);

    // detect change and read data
    for ( i=0; i< 85; i++)
    {
        counter = 0;
        while (readGPIOraw(_gpio) == laststate)
        {
            counter++;
            FrameworkTimer::usleep_s( 1 );
            if (counter == 255)
                break;
        }
        laststate = readGPIOraw(_gpio);
        if (counter == 255)
            break;
        // ignore first 3 transitions
        if ((i >= 4) && (i%2 == 0)) {
            // shove each bit into the storage bytes
            dht22_dat[j/8] <<= 1;
            if (counter > 16)
                dht22_dat[j/8] |= 1;
            j++;
        }
    }

    // check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
    // print it out if data is good
    if ((j >= 40) &&
            (dht22_dat[4] == ((dht22_dat[0] + dht22_dat[1] + dht22_dat[2] + dht22_dat[3]) & 0xFF)) )
    {
        float new_humidity = (dht22_dat[0] * 256 + dht22_dat[1]) / 10.0;
        float new_temp = (((dht22_dat[2] & 0x7F)* 256 + dht22_dat[3]) / 10.0) + _temp_correction;
        /* negative temp */
        if ((dht22_dat[2] & 0x80) != 0)
            new_temp = -new_temp;

        if ( (_timestamp == 0) || // prima lettura sempre buona!
             ( (new_humidity > 0.0) &&    // dato valido?
               (new_humidity < 95.0) &&   // dato valido?
               (fabs(_humidity-new_humidity)<10.0) // delta ragionevole?
               ) )
        {
            _humidity = new_humidity;
            ret = true;
        }
        if ( (_timestamp == 0) || // prima lettura sempre buona!
             ( ( new_temp > -60.0 ) && // dato valido?
               ( new_temp < 60.0 ) &&  // dato valido?
               (fabs(_temp-new_temp)<2.0) // delta ragionevole?
               ) )
        {
            _temp = new_temp;
            ret = true;
        }

        // Skip fake readings (will turn on anti-ice):
        if ( (_humidity == 0) && (_temp == 0) && ret )
            ret = false;
        else
        {
            // Reading was good!
            _timestamp = FrameworkTimer::getTimeEpoc();
        }
    }
#endif

    _logger->logDebug("Sensor reads - t: " + FrameworkUtils::ftostring( _temp ) +
                      " h: " + FrameworkUtils::ftostring( _humidity ) +
                      " - Last successful read: "  + FrameworkUtils::tostring( _timestamp ) );
    return ret;
}
