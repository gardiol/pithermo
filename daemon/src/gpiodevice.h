#ifndef GPIODEVICE_H
#define GPIODEVICE_H

#include <stdint.h>
#include <map>

class GPIODevice
{
public:
    GPIODevice();
    virtual ~GPIODevice();

protected:
    void setGPIOoutput( int gpio_no );
    void setGPIOinput( int gpio_no );
    bool readPGIObool( int gpio_no );
    void writeGPIObool( int gpio_no, bool value );
    uint8_t readGPIOraw( int gpio_no );

private:
    std::map<int, bool> _mode;
    std::map<int, bool> _values;

};

#endif // GPIODEVICE_H
