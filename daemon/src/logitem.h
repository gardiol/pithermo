#ifndef LOGITEM_H
#define LOGITEM_H

#include <stdio.h>
#include <stdint.h>
#include <string>

class LogItem
{
public:
    enum Event {
        NO_EVENT               = 0x0000000000000000,// 0
        START                  = 0x0000000000000001,// 1
        STOP                   = 0x0000000000000002,// 2
        GAS_ON                 = 0x0000000000000004,// 4
        GAS_OFF                = 0x0000000000000008,// 8
        PELLET_ON              = 0x0000000000000010,// 16
        PELLET_OFF             = 0x0000000000000020,// 32
        PELLET_MINIMUM         = 0x0000000000000040,// 64
        PELLET_MODULATION      = 0x0000000000000080,// 128
        PELLET_FLAMEOUT_ON     = 0x0000000000000100,// 256
        PELLET_FLAMEOUT_OFF    = 0x0000000000000200,// 512
        PELLET_HOT             = 0x0000000000000400,// 1024
        PELLET_COLD            = 0x0000000000000800,// 2048
        OVER_TEMP_ON           = 0x0000000000001000,// 4096
        OVER_TEMP_OFF          = 0x0000000000002000,// 8192
        UNDER_TEMP_ON          = 0x0000000000004000,// 16384
        UNDER_TEMP_OFF         = 0x0000000000008000,// 32768
        ANTI_ICE_ON            = 0x0000000000010000,// 65536
        ANTI_ICE_OFF           = 0x0000000000020000,// 128k
        MANUAL_MODE            = 0x0000000000040000,// 256k
        AUTO_MODE              = 0x0000000000080000,// 512k
        MIN_TEMP_UPDATE        = 0x0000000000100000,// 1024k
        MAX_TEMP_UPDATE        = 0x0000000000200000,// 2048k
        PROGRAM_UPDATE         = 0x0000000000400000,// 4096k
        ACTIVATED              = 0x0000000000800000,// 8192k
        DEACTIVATED            = 0x0000000001000000,// 16384k
        HYST_UPDATE            = 0x0000000002000000,// 32768k
        SMART_TEMP_ON          = 0x0000000004000000,// 65536k
        SMART_TEMP_OFF         = 0x0000000008000000,// 128m
        MANUAL_OFF_TIME        = 0x0000000010000000,// 256m
        MANUAL_OFF_TIME_UPDATE = 0x0000000020000000,// 512m
        EXCESSIVE_OVERTEMP_ON  = 0x0000000040000000,// 1024m
        EXCESSIVE_OVERTEMP_OFF = 0x0000000080000000,// 2048m
        EXCESSIVE_OVERTEMP_UPD = 0x0000000100000000,// 4096m
        EXTERNAL_MODE          = 0x0000002000080000 // 8096m
    };

    static uint32_t getSize();

    LogItem( Event t );
    LogItem( FILE* file );
    LogItem( const LogItem& other );
    LogItem();
    void read( FILE* file );
    void write(FILE* file );
    void writeText( FILE* file );

    void operator=(const LogItem& other );

    uint64_t getTime() const
    {
        return _time;
    }

    uint64_t getEvent() const
    {
        return _event;
    }

    bool isValid() const
    {
        return _valid;
    }

private:
    uint64_t _time;
    uint64_t _event;
    bool _valid;
};

#endif // LOGITEM_H
