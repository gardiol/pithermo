#include "tempsensor.h"

#include "frameworkutils.h"
#include "debugprint.h"

#include <math.h>
#include <time.h>

#ifdef DEMO
#include <demowiringpi.h>
#define DEMO_MODE true
#else
#include <wiringPi.h>
#define DEMO_MODE false
#endif

TempSensor::TempSensor(Logger *l, uint8_t gpio, float temp_correction):
    _logger(l),
    _temp(0.0),
    _humidity(0.0),
    _timestamp(0),
    _gpio(gpio),
    _temp_correction(temp_correction),
    _at_lease_one_read_ok(false)
{
    _timer.setLoopTime( 3000 * 1000 );
}

TempSensor::~TempSensor()
{
}

void TempSensor::printStatus()
{
    _logger->logDebug("Sensor reads - t: " + FrameworkUtils::ftostring( _temp ) +
                      " h: " + FrameworkUtils::ftostring( _humidity ) +
                      " - Last successful read: "  + FrameworkUtils::utostring( _timestamp ) );
}


bool TempSensor::readSensor()
{
    bool ret = false;
#ifdef DEMO
    FILE* demo_file = fopen("demo.tmp", "r");
    if ( demo_file != nullptr )
    {
        fscanf( demo_file, "%f %f", &_temp, &_humidity );
        fclose(demo_file);
    }
    else
    {
        _temp = 20;
        _humidity = 50.0f;
    }
    ret = _at_lease_one_read_ok = true;
    _timestamp = FrameworkTimer::getTimeEpoc();
#else
    if ( !_timer.isRunning() || _timer.elapsedLoop() )
    {
        int pin = _gpio;
        uint8_t laststate = HIGH;
        uint8_t counter = 0;
        int dht22_dat[5] = {0,0,0,0,0};
        uint8_t j = 0, i;

        // pull pin down for 18 milliseconds
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
        delay(18);
        // then pull it up for 40 microseconds
        digitalWrite(pin, HIGH);
        delayMicroseconds(40);
        // prepare to read the pin
        pinMode(pin, INPUT);

        // detect change and read data
        for ( i=0; i< 85; i++)
        {
            counter = 0;
            while (static_cast<uint8_t>(digitalRead(pin)) == laststate)
            {
                counter++;
                delayMicroseconds(1);
                if (counter == 255)
                    break;
            }
            laststate = static_cast<uint8_t>(digitalRead(pin));
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
            float new_humidity = (dht22_dat[0] * 256 + dht22_dat[1]) / 10.0f;
            float new_temp = (((dht22_dat[2] & 0x7F)* 256 + dht22_dat[3]) / 10.0f) + _temp_correction;
            // negative temp
            if ((dht22_dat[2] & 0x80) != 0)
                new_temp = -new_temp;

            if ( (_timestamp == 0) || // prima lettura sempre buona!
                 ( (new_humidity > 0.0f) &&    // dato valido?
                   (new_humidity < 95.0f) &&   // dato valido?
                   (fabs(_humidity-new_humidity)<10.0f) // delta ragionevole?
                   ) )
            {
                _humidity = new_humidity;
                ret = true;
            }
            if ( (_timestamp == 0) || // prima lettura sempre buona!
                 ( ( new_temp > -60.0f ) && // dato valido?
                   ( new_temp < 60.0f ) &&  // dato valido?
                   (fabs(_temp-new_temp)<2.0f) // delta ragionevole?
                   ) )
            {
                _temp = new_temp;
                ret = true;
            }

            // Skip fake readings (will turn on anti-ice):
            if ( ( fabs(_humidity) < 0.05f) && ( fabs(_temp) < 0.05f) && ret )
                ret = false;
        }
        if ( ret )
        {
            _at_lease_one_read_ok = true;
            _timestamp = FrameworkTimer::getTimeEpoc();
        }
        _timer.reset();
    }
    else
        ret = _at_lease_one_read_ok; // not yet a first read

#endif
    return ret;
}
