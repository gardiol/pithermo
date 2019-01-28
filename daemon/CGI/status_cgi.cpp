
#include <stdint.h>
#include <list>
#include <string.h>
#include "frameworkutils.h"
#include "frameworktimer.h"
#include "sharedmemory.h"
#include "sharedstatus.h"

using namespace FrameworkLibrary;

int main( int , char** )
{
    int ret = 255;
    SharedMemory shared_status("SharedStatus", SharedStatusKey, sizeof( SharedStatus ), SharedMemory::read_only_shm );

    printf("Content-type: text/plain\n\n");
    if ( shared_status.isReady() )
    {
        SharedStatus status;
        memcpy( &status, shared_status.getReadPtr(), shared_status.getSharedSize() );
        if ( status.marker == SharedStatusMarker )
        {
            printf("%llu %c %c %c %c %c %c %c %c %f %f %f %f %f %f %f %f %c %f",
                   static_cast<unsigned long long int>(status.last_update_stamp), //0
                   status.active ? '1' : '0',//1
                   status.anti_ice_active ? '1' : '0',//2
                   status.manual_mode ? '1' : '0',//3
                   status.pellet_on ? '1' : '0',//4
                   status.pellet_minimum ? '1' : '0',//5
                   status.pellet_hot ? '1' : '0',//6
                   status.pellet_flameout ? '1' : '0',//7
                   status.gas_on ? '1' : '0',//8
                   static_cast<double>(status.max_temp),//9
                   static_cast<double>(status.min_temp),//10
                   static_cast<double>(status.hysteresis_max),//11
                   static_cast<double>(status.hysteresis_min),//12
                   static_cast<double>(status.temp_int),//13
                   static_cast<double>(status.humidity_int),//14
                   static_cast<double>(status.temp_ext),//15
                   static_cast<double>(status.humidity_ext),//16
                   status.smart_temp_on ? '1' : '0',//17
                   static_cast<double>(status.smart_temp) );//18

            ret = (FrameworkTimer::getCurrentTime() - status.last_update_stamp) < 1000000;
        }
    }
    return ret;
}
