
#include "frameworktimer.h"
#include <string.h>

using namespace FrameworkLibrary;

static int demo_store[256];

int  wiringPiSetup(void)
{
    memset( demo_store, 0, 256 * sizeof(int) );
    return 0;
}

void pinMode(int pin, int mode)
{

}

int digitalRead(int pin)
{
    if ( pin > 0 && pin < 256 )
        return demo_store[pin];
    return 0;
}

void digitalWrite(int pin, int value)
{
    if ( pin > 0 && pin < 256 )
        demo_store[pin] = value;
}

void delay(unsigned int howLong)
{
    FrameworkTimer::msleep_s( static_cast<int32_t>(howLong) );
}

void delayMicroseconds(unsigned int howLong)
{
    FrameworkTimer::usleep_s( static_cast<int64_t>(howLong) );
}
