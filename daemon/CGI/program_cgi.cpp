
#include <iostream>
#include <string.h>
#include "../framework/frameworkutils.h"
#include "../framework/frameworktimer.h"
#include "sharedmemory.h"
#include "sharedstatus.h"

#include "udpsocket.h"

using namespace FrameworkLibrary;

int main( int , char** )
{
    int ret = 255;
    printf("Content-type: text/plain\n\n");

    std::string input;
    std::getline( std::cin, input );

    if ( input.length() > 0 )
    {
        UdpSocket sender("CommandSend","127.0.0.1","",5555,0);
        input = "program:"+input;
        if ( sender.activateInterface() )
        {
            if ( sender.writeData( input.c_str(), static_cast<int>(input.length()) ) == static_cast<int>(input.length()) )
            {
                FrameworkTimer::sleep_s(2);
            }
        }
    }

    SharedMemory shared_status("SharedStatus", SharedStatusKey, sizeof( SharedStatus ), SharedMemory::read_only_shm );
    if ( shared_status.isReady() )
    {
        SharedStatus status;
        memcpy( &status, shared_status.getReadPtr(), shared_status.getSharedSize() );
        if ( status.marker == SharedStatusMarker )
        {
            printf("%u %d %u ", status.day, status.hour, status.half );
            for ( int s = 0; s < 24*2*7; s++ )
                putc( status.program[s], stdout );
            ret = 0;
        }
    }
    return ret;
}
