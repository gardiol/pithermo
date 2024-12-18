
#include <iostream>
#include "../src/sharedmemory.h"
#include "../src/sharedstatus.h"
#include "../src/pithermotimer.h"
#include "../src/pithermoutils.h"
#include "../src/command.h"

int main( int , char** )
{
    int ret = 255;
    printf("Content-type: text/plain\n\n");

    std::string input;
    std::getline( std::cin, input );

    // Format: "command param" where param is a number
    std::vector<std::string> tokens = PithermoUtils::string_split( input, " " );
    Command::CmdType cmd = Command::INVALID;
    double param = 0;

    if ( tokens.size() > 0 )
    {
        if ( tokens.size() > 1 )
            param = PithermoUtils::string_tod( tokens[1] );
        if ( tokens[0] == "activate" )
            cmd = Command::ACTIVATE;
        else if ( tokens[0] == "deactivate" )
            cmd = Command::DEACTIVATE;
        else if ( tokens[0] == "pellet-on" )
            cmd = Command::PELLET_ON;
        else if ( tokens[0] == "pellet-off" )
            cmd = Command::PELLET_OFF;
        else if ( tokens[0] == "pellet-minimum-on" )
            cmd = Command::PELLET_MINIMUM_ON;
        else if ( tokens[0] == "pellet-minimum-off" )
            cmd = Command::PELLET_MINIMUM_OFF;
        else if ( tokens[0] == "gas-on" )
            cmd = Command::GAS_ON;
        else if ( tokens[0] == "gas-off" )
            cmd = Command::GAS_OFF;
        else if ( tokens[0] == "manual" )
            cmd = Command::MANUAL;
        else if ( tokens[0] == "auto" )
            cmd = Command::AUTO;
        else if ( tokens[0] == "external" )
            cmd = Command::EXTERNAL;
        else if ( tokens[0] == "smart-temp-on" )
            cmd = Command::SMART_TEMP_ON;
        else if ( tokens[0] == "smart-temp-off" )
            cmd = Command::SMART_TEMP_OFF;
        else if ( tokens[0] == "min-temp" )
            cmd = Command::SET_MIN_TEMP;
        else if ( tokens[0] == "max-temp" )
            cmd = Command::SET_MAX_TEMP;
        else if ( tokens[0] == "set-hyst-min" )
            cmd = Command::SET_HISTERESYS_MIN;
        else if ( tokens[0] == "set-hyst-max" )
            cmd = Command::SET_HISTERESYS_MAX;
        else if ( tokens[0] == "set-hyst-max" )
            cmd = Command::SET_HISTERESYS_MAX;
        else if ( tokens[0] == "set-mot" )
            cmd = Command::SET_MANUAL_OFF_TIME;
        else if ( tokens[0] == "exc-temp" )
            cmd = Command::SET_EXCESSIVE_OVERTEMP_TS;
            //                   PROGRAM                   = 0x0E,
            //                   TEMPLATE_SET              = 0x12,
    }

    SharedMemory shared_status("SharedStatus", SharedStatusKey, sizeof( SharedStatus ), SharedMemory::read_write_nocreate_shm );
    SharedStatus* write_status = (SharedStatus*)shared_status.getWritePtr();

    if ( write_status->marker == SharedStatusMarker )
    {
        // Book our intention to write a command
        write_status->command_queue_write_busy++;

        // wait up to 10ms for write permission
        int32_t wait_cycles = 100;
        while ( (wait_cycles-- > 0) &&
                (write_status->command_queue_write_busy > 1) )
        {
            PithermoTimer::msleep_s(1);
        }

        if ( write_status->command_queue_write_busy == 1 )
        {

            write_status->command_queue[ write_status->command_queue_write_ptr ].command = cmd;
            write_status->command_queue[ write_status->command_queue_write_ptr ].payload = param;

            write_status->command_queue_write_ptr++;
            if ( write_status->command_queue_write_ptr >= SharedStatusCommandQueueSize )
                write_status->command_queue_write_ptr = 0;

            ret = 0;
        }

        // Whatever we did, it's now over, release the intention to write.
        write_status->command_queue_write_busy--;
    }
    return ret;
}
