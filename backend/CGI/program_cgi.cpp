
#include <iostream>
#include <string.h>
#include "../src/sharedmemory.h"
#include "../src/sharedstatus.h"
#include "../src/pithermotimer.h"
#include "../src/command.h"

int main( int , char** )
{
    int ret = 255;
    printf("Content-type: text/plain\n\n");

    std::string input;
    std::getline( std::cin, input );

    SharedMemory shared_status("SharedStatus", SharedStatusKey, sizeof( SharedStatus ), SharedMemory::read_write_nocreate_shm );
    if ( shared_status.isReady() )
    {
        // Do we have a command to send?
        int input_size = input.length();
        if ( input_size > 12 )
        {
            Command::CmdType cmd = Command::INVALID;
            std::string payload_param = "";

            int start = 0;
            if ( input.substr(0, 7) == "program" )
            {
                start = 7;
                cmd = Command::PROGRAM;
            }
            else if ( input.substr(0, 12) == "template-set" )
            {
                start = 12;
                cmd = Command::TEMPLATE_SET;
            }
            payload_param =  input.substr( start ).c_str();

            if ( payload_param.length() <= SharedStatusProgramSize )
            {
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
                        write_status->command_queue[ write_status->command_queue_write_ptr ].payload = 0;
                        char* string_ptr = write_status->command_queue[ write_status->command_queue_write_ptr ].payload_string;
                        memset( string_ptr, 0, SharedStatusProgramSize );
                        memcpy( string_ptr, payload_param.c_str(), payload_param.length() );

                        write_status->command_queue_write_ptr++;
                        if ( write_status->command_queue_write_ptr >= SharedStatusCommandQueueSize )
                            write_status->command_queue_write_ptr = 0;
                    }
                }

                // Whatever we did, it's now over, release the intention to write.
                write_status->command_queue_write_busy--;
            }
        }

        SharedStatus status;
        memcpy( &status, shared_status.getReadPtr(), shared_status.getSharedSize() );
        if ( status.marker == SharedStatusMarker )
        {
            printf("%u %d %u ", status.day, status.hour, status.half );
            for ( int s = 0; s < 24*2*7; s++ )
                putc( status.program[s], stdout );
            putc(' ', stdout);
            for ( unsigned int t = 0; t < SharedStatusNumTemplates; t++ )
            {
                printf("%d %s ", t, strlen(status.templates_names[t]) == 0 ? "-" : status.templates_names[t]);
                for ( int s = 0; s < 24*2; s++ )
                    putc( status.templates[t][s], stdout );
                putc(' ', stdout);
            }
            ret = 0;
        }
    }
    return ret;
}
