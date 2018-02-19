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
    _mode[ gpio_no ] = INPUT;
#endif
}

bool GPIODevice::readPGIObool(int gpio_no)
{ // false : LOW : relé closed - true : HIGH : relé open
    if ( gpio_no > -1 )
    {
#ifndef DEMO
        pinMode( gpio_no, _mode[ gpio_no ] );
        _values[ gpio_no ] = (digitalRead( gpio_no ) == HIGH);
#else
        if ( _values.find( gpio_no ) == _values.end() )
            _values[ gpio_no ] = false;
#endif
        return _values[ gpio_no ];
    }
    return false;
}

void GPIODevice::writeGPIObool(int gpio_no, bool value)
{ // false : LOW : relé closed - true : HIGH : relé open
    if ( gpio_no > -1 )
    {
#ifndef DEMO
        pinMode( gpio_no, OUTPUT);
        digitalWrite( gpio_no, value ? HIGH : LOW );
#endif
        _values[ gpio_no ] = value;
    }
}

uint8_t GPIODevice::readGPIOraw(int gpio_no)
{
    if ( gpio_no > -1 )
    {
#ifdef DEMO
        return 0;
#else
        int read = digitalRead( gpio_no );
        //    if ( read > 255 || read <  0 )
        //        debugPrintError("WiringPi") << "Invalid data from wiringPi library\n";
        return read;
#endif
    }
    return 0;
}
