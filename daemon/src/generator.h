#ifndef GENERATOR_H
#define GENERATOR_H

#include "logger.h"
#include "gpiodevice.h"

class Generator: protected GPIODevice
{
public:
    enum PowerLevel { POWER_LOW, POWER_HIGH };

    Generator(const std::string& n,
              Logger* l,
              int command_gpio,
              int status_gpio,
              int power_gpio,
              LogItem::Event on_event,
              LogItem::Event off_event,
              LogItem::Event low_event,
              LogItem::Event high_event );

    virtual ~Generator();

    bool switchOn();
    bool switchOff();

    bool setPower( PowerLevel pl );

    bool isOn();
    bool isHot();
    bool isLow();

    uint64_t lastOnTime();

    void printStatus();

protected:

private:
    std::string _name;
    Logger* _logger;

    int _command_gpio;
    int _status_gpio;
    int _power_gpio;

    uint64_t _on_since;

    LogItem::Event _on_event;
    LogItem::Event _off_event;
    LogItem::Event _low_event;
    LogItem::Event _high_event;
};

#endif // GENERATOR_H
