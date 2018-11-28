
#include <stdint.h>
#include <list>
#include "../framework/frameworkutils.h"
#include "../framework/frameworktimer.h"
#include "../src/logger.h"
#include "../src/logitem.h"
#include "../src/history.h"

using namespace FrameworkLibrary;

#define DAY_OFFSET 24*60*60

static uint64_t from_day = 0;
static uint64_t to_day = 0;

bool parse_POST()
{
    unsigned long long int from = 0, to = 0;
    if ( fscanf( stdin, "%llu:%llu", &from, &to) == 2 )
    {
        if ( (from > 0) && (to > 0) && (from < to) )
        {
            from_day = from - from % DAY_OFFSET;
            to_day  = to - to % DAY_OFFSET + DAY_OFFSET;
            return true;
        }
    }
    return false;
}

int main( int , char** )
{
    std::string events_file = "../events";
    std::string history_file = "../history";
    int ret = 255;
    printf("Content-type: text/plain\n\n");

    if ( FrameworkUtils::fileExist( events_file ) &&
         FrameworkUtils::fileExist( history_file ) )
    {
        if ( parse_POST() )
        {
            Logger logger( events_file );
            History history( history_file );

            uint32_t total_pellet_on_time = 0;
            uint32_t total_pellet_low_time = 0;
            uint32_t total_gas_on_time = 0;

            bool prev_valid = false;
            bool prev_pellet_on = false;
            bool prev_pellet_minimum_on = false;
            bool prev_gas_on = false;

            for ( uint64_t day = from_day; day < to_day; day += DAY_OFFSET )
            {
                uint64_t end = day + DAY_OFFSET;
                std::list<HistoryItem> datas;
                uint32_t pellet_on_time = 0;
                uint32_t pellet_low_time = 0;
                uint32_t gas_on_time = 0;
                float min_t = 0;
                float max_t = 0;
                float avg_t = 0;
                float min_et = 0;
                float max_et = 0;
                float avg_et = 0;
                if ( logger.calculateStats( day, end,
                                            prev_valid,
                                            prev_pellet_on,
                                            prev_pellet_minimum_on,
                                            prev_gas_on,
                                            pellet_on_time, pellet_low_time, gas_on_time ) )
                {
                    total_pellet_on_time += pellet_on_time;
                    total_pellet_low_time += pellet_low_time;
                    total_gas_on_time += gas_on_time;
                     if ( history.calculateStats( day, end, min_t, max_t, avg_t, min_et, max_et, avg_et ) )
                     {
                        printf("%llu %lu %lu %lu %f %f %f %f %f %f\n",
                               static_cast<unsigned long long int>(day),
                               static_cast<unsigned long int>(pellet_on_time),
                               static_cast<unsigned long int>(pellet_low_time),
                               static_cast<unsigned long int>(gas_on_time),
                               static_cast<double>(min_t),
                               static_cast<double>(max_t),
                               static_cast<double>(avg_t),
                               static_cast<double>(min_et),
                               static_cast<double>(max_et),
                               static_cast<double>(avg_et) );
                     }
                }
            }
            printf("Totals %lu %lu %lu",
                   static_cast<unsigned long int>(total_pellet_on_time),
                   static_cast<unsigned long int>(total_pellet_low_time),
                   static_cast<unsigned long int>(total_gas_on_time) );
        }
    }
    return ret;
}
