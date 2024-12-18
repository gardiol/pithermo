#include "generator.h"

#include "pithermotimer.h"

Generator::Generator(const std::string &n,
                     Logger *l,
                     int command_gpio,
                     int status_gpio,
                     int power_gpio,
                     LogItem::Event on_event,
                     LogItem::Event off_event,
                     LogItem::Event low_event,
                     LogItem::Event high_event,
                     LogItem::Event missed_start_event,
                     LogItem::Event missed_start_clear_event,
                     uint64_t max_hot_timeout):
    _name(n),
    _logger(l),
    _command_gpio( command_gpio ),
    _status_gpio( status_gpio ),
    _power_gpio( power_gpio ),
    _on_since(0),
    _max_hot_timeout( max_hot_timeout ),
    _missed_start(false),
    _on_event(on_event),
    _off_event(off_event),
    _low_event(low_event),
    _high_event(high_event),
    _missed_start_event(missed_start_event),
    _missed_start_clear_event(missed_start_clear_event),    
    _quiet(true)
  #ifdef DEMO
  , _off_since(0)
  , _temp_hot(false)
  #endif
{
    setGPIOoutput( _command_gpio );
    setGPIOoutput( _power_gpio );
    setGPIOinput( _status_gpio );
    _logger->logDebug(_name + " starting..." );
    if ( isOn() )
    {
        _logger->logDebug(_name + " seems to be ON" );
        switchOn();
        if ( isLow() )
            setPower( POWER_LOW );
        else
            setPower( POWER_HIGH );
    }
    else
    {
        _logger->logDebug(_name + " seems to be OFF" );
        switchOff();
    }
    _logger->logDebug(_name + " started." );
    _quiet = false;
}

Generator::~Generator()
{
}

bool Generator::switchOn(bool force_power_high)
{    // on = LOW/false
    if ( _on_since == 0 )
    {
        if ( force_power_high )
            // Always start from a known power level.
            // Failing to do so can cause a mess when calculating statistics
            // This is not a problem, anyway,
            setPower( POWER_HIGH );
        _on_since = PithermoTimer::getTimeEpoc();
#ifdef DEMO
        _off_since = 0;
#endif
        if ( !_quiet )
        {
            _logger->logEvent( _on_event );
            _logger->logDebug( _name + " ON");
        }
    }
    writeGPIObool( _command_gpio, false );
    return true;
}

bool Generator::switchOff(bool force_power_high)
{    // off = HIGH/true
    if ( _on_since > 0 )
    {
        _on_since = 0;
        if ( !_quiet )
        {
            _logger->logEvent( _off_event );
            _logger->logDebug( _name + " OFF");
        }
#ifdef DEMO
        _off_since = PithermoTimer::getTimeEpoc();
#endif
        if ( force_power_high )
            // Restore power status to known value on shutoff,
            // Failing to do so can cause a mess when calculating statistics
            setPower( POWER_HIGH );
    }
    writeGPIObool( _command_gpio, true );
    return true;
}

bool Generator::setPower(Generator::PowerLevel pl)
{   // Relé close : LOW/true : power low
    // Relé open : HIGH/false : power high
    writeGPIObool( _power_gpio, pl != POWER_LOW );
    if ( pl == POWER_LOW )
    {
        if ( !_quiet )
        {
            if ( _low_event != LogItem::NO_EVENT )
                _logger->logEvent( _low_event );
            _logger->logDebug( _name + " LOW");
        }
    }
    else
    {
        if ( !_quiet )
        {
            if ( _low_event != LogItem::NO_EVENT )
                _logger->logEvent( _high_event );
            _logger->logDebug( _name + " HIGH");
        }
    }
	return true;
}

bool Generator::isLow()
{
    bool x = !readPGIObool( _power_gpio );
    return x;
}

bool Generator::checkHotTimeout( uint64_t now )
{
    // Ignore if timeout is zero
    if (_max_hot_timeout > 0)
    {   // Generator is ON, but is not HOT we might have a problem...
        if ( isOn() && !isHot() )
        {   // We have not detected a problem yet...
            if ( !_missed_start )
            {   // Check timeout on hot...
                if ( (_on_since + _max_hot_timeout) < now )
                {   // Generator has NOT turned on properly!!!
                    if ( !_quiet )
                    {
                        _logger->logDebug("missed start detected - "+_name+" was never hot after startup time");
                        _logger->logEvent( _missed_start_event );
                    }
                    _missed_start = true;
                }
            }
        }   // If it's OFF (or ON but now HOT) and we have had a problem, we should reset it now:
        else if ( _missed_start )
        {
            if ( !_quiet )
            {
                _logger->logDebug("missed start cleared - "+_name );
                _logger->logEvent( _missed_start_clear_event );
            }
            _missed_start = false;
        }
    }
    return _missed_start;
}

bool Generator::isOn()
{
    bool x = !readPGIObool( _command_gpio );
    return x;
}

bool Generator::isHot()
{
#ifndef DEMO
    // HIGH: mandata fredda, termostato off, relé chiuso, 3.3V
    // LOW: mandata calda, termostato on, relé aperto, 0V
    bool fdb = !readPGIObool( _status_gpio );
    return fdb;
#else
    if ( _on_since > 0 )
        return (PithermoTimer::getTimeEpoc() - _on_since) > 10;
    else if ( _off_since > 0 )
        return (PithermoTimer::getTimeEpoc() - _off_since) < 10;
    return false;
#endif
}

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
