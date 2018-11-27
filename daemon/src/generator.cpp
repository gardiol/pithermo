#include "generator.h"

#include "frameworktimer.h"

using namespace FrameworkLibrary;

Generator::Generator(const std::string &n,
                     Logger *l,
                     int command_gpio,
                     int status_gpio,
                     int power_gpio,
                     LogItem::Event on_event,
                     LogItem::Event off_event,
                     LogItem::Event low_event,
                     LogItem::Event high_event):
    _name(n),
    _logger(l),
    _command_gpio( command_gpio ),
    _status_gpio( status_gpio ),
    _power_gpio( power_gpio ),
    _on_event(on_event),
    _off_event(off_event),
    _low_event(low_event),
    _high_event(high_event)
{
    setGPIOoutput( _command_gpio );
    setGPIOoutput( _power_gpio );
    setGPIOinput( _status_gpio );
    if ( isOn() )
    {
        switchOn();
        if ( isLow() )
            setPower( POWER_LOW );
        else
            setPower( POWER_HIGH );
    }
    else
        switchOff();
}

Generator::~Generator()
{
}

bool Generator::switchOn()
{    // on = LOW/false
    if ( _on_since == 0 )
        _on_since = FrameworkTimer::getTimeEpoc();
    writeGPIObool( _command_gpio, false );
    _logger->logEvent( _on_event );
    _logger->logDebug( _name + " ON");
    return true;
}

bool Generator::switchOff()
{    // off = HIGH/true
    if ( _on_since > 0 )
        _on_since = 0;
    writeGPIObool( _command_gpio, true );
    _logger->logEvent( _off_event );
    _logger->logDebug( _name + " OFF");
    return true;
}

bool Generator::setPower(Generator::PowerLevel pl)
{   // Relé close : LOW/true : power low
    // Relé open : HIGH/false : power high
    writeGPIObool( _power_gpio, pl != POWER_LOW );
    if ( pl == POWER_LOW )
    {
        if ( _low_event != LogItem::NO_EVENT )
            _logger->logEvent( _low_event );
        _logger->logDebug( _name + " LOW");
    }
    else
    {
        if ( _low_event != LogItem::NO_EVENT )
            _logger->logEvent( _high_event );
        _logger->logDebug( _name + " HIGH");
    }
	return true;
}

bool Generator::isLow()
{
    bool x = !readPGIObool( _power_gpio );
    return x;
}

bool Generator::isOn()
{
    bool x = !readPGIObool( _command_gpio );
    return x;
}

bool Generator::isHot()
{
    // HIGH: mandata fredda, termostato off, relé chiuso, 3.3V
    // LOW: mandata calda, termostato on, relé aperto, 0V
    bool fdb = !readPGIObool( _status_gpio );
    return fdb;
}

/*void Generator::newDayResetTimes()
{
    _today_on_time = 0;
    _today_low_time = 0;
    if ( _on_since > 0 )
    {
        uint64_t delta_on = FrameworkTimer::getTimeEpoc() - _on_since;
        _season_on_time += delta_on;
        _on_since = FrameworkTimer::getTimeEpoc();
    }
    if ( _on_low_since > 0 )
    {
        uint64_t delta_on = FrameworkTimer::getTimeEpoc() - _on_low_since;
        _season_low_time += delta_on;
        _on_low_since = FrameworkTimer::getTimeEpoc();
    }
}*/

uint64_t Generator::lastOnTime()
{
    return _on_since;
}

void Generator::printStatus()
{
    _logger->logDebug(_name + " is " + ( isOn() ? "ON" : "OFF" ) );
    if ( _power_gpio > -1 )
        _logger->logDebug(_name + " power is " + ( isLow() ? "LOW" : "HIGH" ) );
    if ( _status_gpio > -1 )
        _logger->logDebug(_name + " status is " + ( isHot() ? "HOT" : "COLD" ) );
}
