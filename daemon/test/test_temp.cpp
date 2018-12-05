
#include <list>
#include <stdio.h>
#include "../src/tempsensor.h"

#ifndef DEMO
#error "This test must compile all in DEMO mode"
#endif


int main()
{
    std::list<float> temps;
    temps.push_back( 10.0f );
    temps.push_back( 10.1f );
    temps.push_back( 10.2f );
    temps.push_back( 10.3f );
    temps.push_back( 10.4f );
    temps.push_back( 10.5f );
    temps.push_back( 10.6f );
    temps.push_back( 10.7f );
    temps.push_back( 10.8f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.9f );
    temps.push_back( 10.8f );
    temps.push_back( 10.9f );
    temps.push_back( 10.8f );
    temps.push_back( 10.9f );
    temps.push_back( 10.8f );
    temps.push_back( 10.9f );
    temps.push_back( 10.8f );
    temps.push_back( 10.9f );
    temps.push_back( 10.8f );
    temps.push_back( 10.9f );
    temps.push_back( 10.8f );
    temps.push_back( 10.9f );
    temps.push_back( 10.8f );
    temps.push_back( 10.9f );
    temps.push_back( 10.8f );
    temps.push_back( 10.9f );
    temps.push_back( 10.8f );
    temps.push_back( 10.9f );
    temps.push_back( 10.8f );
    temps.push_back( 10.9f );
    temps.push_back( 10.8f );
    temps.push_back( 10.7f );
    temps.push_back( 10.6f );
    temps.push_back( 10.5f );
    temps.push_back( 10.4f );
    temps.push_back( 10.3f );
    temps.push_back( 10.2f );
    temps.push_back( 10.1f );
    temps.push_back( 10.0f );
    temps.push_back( 10.0f );
    temps.push_back( 10.0f );

    TempSensor sens( nullptr, 0, 0);

    while ( temps.size() > 0 )
    {
        float t = temps.front();
        FILE* demo_file = fopen("demo.tmp", "w" );
        fprintf( demo_file, "%f %f", static_cast<double>(t), 50.0 );
        fclose( demo_file );

        if ( sens.readSensor() )
        {
            float temp = sens.getTemp();
            printf("Input: %f Output: %f\n", static_cast<double>(t), static_cast<double>(temp));
        }
        else
            printf("Sensor read error\n");

        temps.pop_front();
    }
    return 0;
}
