#include "gpiodevice.h"

#ifdef DEMO
#include <demowiringpi.h>
#define DEMO_MODE true
#else
#include <wiringPi.h>
#define DEMO_MODE false
#endif

GPIODevice::GPIODevice()
{

}

GPIODevice::~GPIODevice()
{

}

void GPIODevice::setGPIOoutput(int gpio_no)
{
    _mode[ gpio_no ] = OUTPUT;
}

void GPIODevice::setGPIOinput(int gpio_no)
{
    _mode[ gpio_no ] = INPUT;
}

bool GPIODevice::readPGIObool(int gpio_no)
{ // false : LOW : relé closed - true : HIGH : relé open
    if ( gpio_no > -1 )
    {
        pinMode( gpio_no, _mode[ gpio_no ] );
        _values[ gpio_no ] = (digitalRead( gpio_no ) == HIGH);
        return _values[ gpio_no ];
    }
    return false;
}

void GPIODevice::writeGPIObool(int gpio_no, bool value)
{ // false : LOW : relé closed - true : HIGH : relé open
    if ( gpio_no > -1 )
    {
        pinMode( gpio_no, OUTPUT);
        digitalWrite( gpio_no, value ? HIGH : LOW );
    }
}

uint8_t GPIODevice::readGPIOraw(int gpio_no)
{
    if ( gpio_no > -1 )
    {
        int read = digitalRead( gpio_no );
        return static_cast<uint8_t>(read);
    }
    return 0;
}
