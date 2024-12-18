#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>

#include <string>

class Command
{
public:
    enum CmdType { INVALID                   = 0x00,
                   ACTIVATE                  = 0x01,
                   DEACTIVATE                = 0x02,
                   PELLET_ON                 = 0x03,
                   PELLET_OFF                = 0x04,
                   PELLET_MINIMUM_ON         = 0x05,
                   PELLET_MINIMUM_OFF        = 0x06,
                   GAS_ON                    = 0x07,
                   GAS_OFF                   = 0x08,
                   MANUAL                    = 0x09,
                   SET_MIN_TEMP              = 0x0A,
                   SET_MAX_TEMP              = 0x0B,
                   SET_HISTERESYS_MAX        = 0x0C,
                   SET_HISTERESYS_MIN        = 0x0D,
                   PROGRAM                   = 0x0E,
                   SMART_TEMP_ON             = 0x10,
                   SMART_TEMP_OFF            = 0x11,
                   TEMPLATE_SET              = 0x12,
                   AUTO                      = 0x13,
                   SET_MANUAL_OFF_TIME       = 0x14,
                   SET_EXCESSIVE_OVERTEMP_TS = 0x15,
                   EXTERNAL                  = 0x16
    };

    Command(uint32_t command_id, double param , const std::string &string);
    ~Command();

    CmdType command() const;
    double getParam() const;
    std::string getParamString() const;

private:
    CmdType _command;
    double _param;
    std::string _param_str;

};

#endif // COMMAND_H
