#include "gpiodevice.h"

#ifndef DEMO
#include <wiringPi.h>
#endif

GPIODevice::GPIODevice()
{

}

GPIODevice::~GPIODevice()
{

}

void GPIODevice::setGPIOoutput(int gpio_no)
{
#ifndef DEMO
    _mode[ gpio_no ] = OUTPUT;
#endif
}

void GPIODevice::setGPIOinput(int gpio_no)
{
#ifndef DEMO
    _mode[ gpio_no ] = OUTPUT;
#endif
}

bool GPIODevice::readPGIObool(int gpio_no)
{
#ifndef DEMO
    pinMode( gpio_no, _mode[ gpio_no ] );
    _values[ gpio_no ] = (digitalRead( g ) == HIGH);
#else
    if ( _values.find( gpio_no ) = _values.end() )
        _values[ gpio_no ] = false;
#endif
    return _values[ gpio_no ];
}

void GPIODevice::writeGPIObool(int gpio_no, bool value)
{ // false : LOW : relé closed - true : HIGH : relé open
#ifndef DEMO
    pinMode( gpio_no, OUTPUT);
    digitalWrite( gpio_no, value ? HIGH : LOW );
#endif
    _values[ gpio_no ] = value;
    return _values[ gpio_no ];
}

uint8_t GPIODevice::readGPIOraw(int gpio_no)
{
#ifdef DEMO
    return 0;
#else
    int read = digitalRead( gpio_no );
    if ( read > 255 || read <  0 )
        debugPrintError("WiringPi") << "Invalid data from wiringPi library\n";
    return read;
#endif
}