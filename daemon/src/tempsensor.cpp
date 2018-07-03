#include "tempsensor.h"

#include "frameworkutils.h"

#include <math.h>
#include <time.h>
#include <wiringPi.h>

TempSensor::TempSensor(Logger *l, int gpio, float temp_correction):
    _logger(l),
    _temp(0.0),
    _humidity(0.0),
    _timestamp(0),
    _gpio(gpio),
    _temp_correction(temp_correction),
    _at_lease_one_read_ok(false)
{
    _timer.setLoopTime( 5000 * 1000 );
}

TempSensor::~TempSensor()
{
}

float TempSensor::getTemp() const
{
    return _temp;
}

float TempSensor::getHimidity() const
{
    return _humidity;
}

void TempSensor::printStatus()
{
    _logger->logDebug("Sensor reads - t: " + FrameworkUtils::ftostring( _temp ) +
                      " h: " + FrameworkUtils::ftostring( _humidity ) +
                      " - Last successful read: "  + FrameworkUtils::tostring( _timestamp ) );
}


bool TempSensor::readSensor()
{
    bool ret = false;
    if ( !_timer.isRunning() || _timer.elapsedLoop() )
    {
#ifndef DEMO
        FrameworkTimer looper;
        int32_t seqbuf[40];
        uint8_t rcvbuf[5] = {0,0,0,0,0};
        int32_t max_value = 0;
        int32_t min_value = 3 * 1000 * 1000;

        // Request phase
        pinMode(_gpio, OUTPUT);
        // High
        digitalWrite(_gpio, HIGH);
        delay(250);
        // Low
        digitalWrite(_gpio, LOW);
        delay(20);
        // High
        digitalWrite(_gpio, HIGH);
        delayMicroseconds(35);

        // Reading phase
        pinMode(_gpio, INPUT);
        delayMicroseconds(10);

        // Read LOW for 80us
        looper.setLoopTime(80);
        while( !looper.elapsedLoop() && (digitalRead(_gpio) == LOW) )
            delayMicroseconds(1);
        if ( !looper.elapsedLoop() )
        {
            // Read HIGH for 80us
            looper.setLoopTime(80);
            while( !looper.elapsedLoop() && (digitalRead(_gpio) == HIGH) )
                delayMicroseconds(1);
            if ( !looper.elapsedLoop() )
            {
                bool timeout = false;
                // Read output, on 40bits (5 bytes)
                for (int front_index=0; !timeout && (front_index<40); front_index++)
                {
                    // Wait for LOW
                    looper.setLoopTime(80);
                    while( !looper.elapsedLoop() && (digitalRead(_gpio) == LOW) )
                        delayMicroseconds(1);
                    timeout = looper.elapsedLoop();
                    if ( !timeout )
                    {
                        // Start from 0
                        seqbuf[front_index] = 0;

                        looper.setLoopTime(80);
                        while( !looper.elapsedLoop() && (digitalRead(_gpio) == HIGH) )
                        {
                            seqbuf[front_index]++;
                            delayMicroseconds(1);
                        }
                        timeout = looper.elapsedLoop();
                    }

                    if(seqbuf[front_index] > max_value)
                        max_value = seqbuf[front_index];
                    if(seqbuf[front_index] < min_value)
                        min_value = seqbuf[front_index];
                }
                if ( !timeout )
                {
                    int32_t mean = (min_value + max_value) / 2;
                    for (int front_index=0; front_index<40; front_index++)
                    {
                        if (seqbuf[front_index] > mean)
                            seqbuf[front_index] = 1;
                        else
                            seqbuf[front_index] = 0;
                    }

                    for ( int loopCnt=0; loopCnt<5; loopCnt++)
                    {
                        rcvbuf[loopCnt] |= (seqbuf[loopCnt*8] << 7);
                        rcvbuf[loopCnt] |= (seqbuf[loopCnt*8+1] << 6);
                        rcvbuf[loopCnt] |= (seqbuf[loopCnt*8+2] << 5);
                        rcvbuf[loopCnt] |= (seqbuf[loopCnt*8+3] << 4);
                        rcvbuf[loopCnt] |= (seqbuf[loopCnt*8+4] << 3);
                        rcvbuf[loopCnt] |= (seqbuf[loopCnt*8+5] << 2);
                        rcvbuf[loopCnt] |= (seqbuf[loopCnt*8+6] << 1);
                        rcvbuf[loopCnt] |= seqbuf[loopCnt*8+7];
                    }

                    // Checksum check and cikkect results
                    if (rcvbuf[4] == ((rcvbuf[0]+rcvbuf[1]+rcvbuf[2]+rcvbuf[3]) & 0xFF) )
                    {
                        float new_humidity = (float) ( (((int16_t)0x0000)) | (((int16_t)rcvbuf[0])<<8) | ((int16_t)rcvbuf[1]) );
                        float new_temp = (float) ( (((int16_t)0x0000)) | (((int16_t)(rcvbuf[2] & 0x7F))<<8) | ((int16_t)rcvbuf[3]) );
                        if( rcvbuf[2] & 0x80)
                            new_temp = -new_temp;

                        new_humidity *= 0.1;
                        new_temp *= 0.1;

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
                    }
                }
            }
        }



        /*        // then pull it up for 40 microseconds
        digitalWrite(pin, HIGH);
        delayMicroseconds(40);
        // prepare to read the pin
        pinMode(pin, INPUT);



        int pin = 1;
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
            while ((uint8_t)(digitalRead(pin)) == laststate)
            {
                counter++;
                delayMicroseconds(1);
                if (counter == 255)
                    break;
            }
            laststate = (uint8_t)(digitalRead(pin));
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
            // negative temp
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
        }*/
#else
        ret = true;
        _temp = 7; //(_timestamp/60) % 20 + 10;
        _humidity = (_timestamp/3600) % 70 + 25;
#endif
        if ( ret )
        {
            _at_lease_one_read_ok = true;
            _timestamp = FrameworkTimer::getTimeEpoc();
        }
        _timer.reset();
    }
    else
        ret = _at_lease_one_read_ok; // not yet a first read, just return valid
    return ret;
}
