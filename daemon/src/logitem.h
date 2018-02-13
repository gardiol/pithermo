#ifndef LOGITEM_H
#define LOGITEM_H

#include <stdio.h>
#include <stdint.h>
#include <string>

class LogItem
{
public:
    enum Event {
        NO_EVENT            = 0x0000000000000000,// 0
        START               = 0x0000000000000001,// 1
        STOP                = 0x0000000000000002,// 2
        GAS_ON              = 0x0000000000000004,// 4
        GAS_OFF             = 0x0000000000000008,// 8
        PELLET_ON           = 0x0000000000000010,// 16
        PELLET_OFF          = 0x0000000000000020,// 32
        PELLET_MINIMUM      = 0x0000000000000040,// 64
        PELLET_MODULATION   = 0x0000000000000080,// 128
        PELLET_FLAMEOUT_ON  = 0x0000000000000100,// 256
        PELLET_FLAMEOUT_OFF = 0x0000000000000200,// 512
        PELLET_HOT          = 0x0000000000000400,// 1024
        PELLET_COLD         = 0x0000000000000800,// 2048
        OVER_TEMP_ON        = 0x0000000000001000,// 4096
        OVER_TEMP_OFF       = 0x0000000000002000,// 8192
        UNDER_TEMP_ON       = 0x0000000000004000,// 16384
        UNDER_TEMP_OFF      = 0x0000000000008000,// 32768
        ANTI_ICE_ON         = 0x0000000000010000,// 65536
        ANTI_ICE_OFF        = 0x0000000000020000,// 128k
        MANUAL_MODE         = 0x0000000000040000,// 256k
        AUTO_MODE           = 0x0000000000080000,// 512k
        MIN_TEMP_UPDATE     = 0x0000000000100000,// 1024k
        MAX_TEMP_UPDATE     = 0x0000000000200000,// 2048k
        PROGRAM_UPDATE      = 0x0000000000400000 // 4096k
    };

    LogItem( Event t );
    LogItem( FILE* file );
    LogItem( const LogItem& other );
    LogItem();
    void write(FILE* file );

    void operator=(const LogItem& other );

    uint64_t getTime() const
    {
        return _time;
    }

    uint64_t getEvent() const
    {
        return _event;
    }
    static int32_t getSize()
    {
        return sizeof(_time) + sizeof(_event);
    }

    bool isValid() const
    {
        return _valid;
    }

    std::string getTimeStr() const
    {
        return _time_str;
    }

    std::string getEventStr() const
    {
        return _event_str;
    }

private:
    uint64_t _time;
    uint64_t _event;
    std::string _time_str;
    std::string _event_str;
    bool _valid;
};

#endif // LOGITEM_H
