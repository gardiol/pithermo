
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
            printf("%llu %c %c %c %c %c %c %c %c %f %f %f %f %f %f %f %c %f",
                   static_cast<unsigned long long int>(status.last_update_stamp),
                   status.active ? '1' : '0',
                   status.anti_ice_active ? '1' : '0',
                   status.manual_mode ? '1' : '0',
                   status.pellet_on ? '1' : '0',
                   status.pellet_minimum ? '1' : '0',
                   status.pellet_hot ? '1' : '0',
                   status.pellet_flameout ? '1' : '0',
                   status.gas_on ? '1' : '0',
                   static_cast<double>(status.max_temp),
                   static_cast<double>(status.min_temp),
                   static_cast<double>(status.hysteresis),
                   static_cast<double>(status.temp_int),
                   static_cast<double>(status.humidity_int),
                   static_cast<double>(status.temp_ext),
                   static_cast<double>(status.humidity_ext),
                   status.smart_temp_on ? '1' : '0',
                   status.smart_temp);

            ret = (FrameworkTimer::getCurrentTime() - status.last_update_stamp) < 1000000;
        }
    }
    return ret;
}
