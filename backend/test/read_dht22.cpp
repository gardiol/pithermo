
#include "../src/tempsensor.h"
#include "../src/pithermoutils.h"
#include "../src/pithermotimer.h"

int main( int argc, char** argv)
{
    int gpio = PithermoUtils::string_tou( argv[1] );

    TempSensor sensor( NULL, gpio, 0.0f );

    while ( true )
    {
        if ( sensor.readSensor() )
        {
            float temp = sensor.getTemp();
            float humi = sensor.getHumidity();
            uint64_t tstmp = sensor.getTimestamp();
            printf("[%llu] %f° - %f%%\n", (unsigned long long int)tstmp, temp, humi );
        }
        PithermoTimer::msleep_s( 1000 );
    }
    return 0;
}
