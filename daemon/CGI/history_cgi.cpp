
#include <stdint.h>
#include <list>
#include "../framework/frameworkutils.h"
#include "../framework/frameworktimer.h"
#include "../src/history.h"
#include "../src/logger.h"

using namespace FrameworkLibrary;

static uint64_t from_time = 0;
static uint64_t to_time = 0;
static uint32_t n_samples = 0;

bool parse_POST()
{
    if ( fscanf( stdin, "%llu:%llu:%u", (unsigned long long int*)&from_time, (unsigned long long int*)&to_time, (unsigned int*)&n_samples ) == 3 )
        if ( (from_time > 0) && //(from_time <= now) &&
             (to_time > 0) && //(to_time <= now) &&
             (from_time < to_time) &&
             (n_samples > 0) && (n_samples < 100000 ) )
            return true;
    return false;
}

int main( int , char** )
{
    std::string history_file = "../history";
    std::string events_file = "../events";
    int ret = 255;
    printf("Content-type: text/plain\n\n");

    if ( FrameworkUtils::fileExist( history_file ) &&
         FrameworkUtils::fileExist( events_file ) )
    {
        if ( parse_POST() )
        {
            History history( history_file );
            Logger logger( events_file );

            std::list<HistoryItem> history_items;
            std::list<LogItem> events_items;

            if ( history.fetchInterval( from_time, to_time, history_items ) &&
                 logger.fetchInterval( from_time, to_time, events_items ) )
            {
                uint32_t n_items = static_cast<uint32_t>(history_items.size());
                uint32_t skip_items = FrameworkUtils_max<uint32_t>( n_items / n_samples, 1);
                uint32_t n_item = 0;
                std::list<LogItem>::iterator e = events_items.begin();
                bool pellet_on = false;
                bool pellet_off = false;
                bool gas_on = false;
                bool gas_off = false;
                for ( std::list<HistoryItem>::iterator i = history_items.begin(); i != history_items.end(); ++i )
                {
                    bool skip_events = false;
                    uint64_t history_time = (*i).getTime();
                    while ( (e != events_items.end()) && !skip_events )
                    {
                        if ( (*e).getTime() <= history_time )
                        {
                            if ( (*e).getEvent() == LogItem::PELLET_ON )
                                pellet_on = true;
                            if ( (*e).getEvent() == LogItem::PELLET_OFF )
                                pellet_off = true;
                            if ( (*e).getEvent() == LogItem::GAS_ON )
                                gas_on = true;
                            if ( (*e).getEvent() == LogItem::GAS_OFF )
                                gas_off = true;
                            ++e;
                        }
                        else
                            skip_events = true;
                    }

                    if ( (n_item % skip_items) == 0 )
                    {
                        printf("%llu %f %f %f %f %d %d %d %d\n",
                               static_cast<unsigned long long int>(history_time),
                               static_cast<double>((*i).getTemp()),
                               static_cast<double>((*i).getHumidity()),
                               static_cast<double>((*i).getExtTemp()),
                               static_cast<double>((*i).getExtHumidity()),
                               pellet_on ? 1:0,
                               pellet_off ? 1:0,
                               gas_on ? 1:0,
                               gas_off ? 1:0);
                        gas_off = gas_on = pellet_off = pellet_on = false;
                    }
                    n_item++;
                }
                ret = 0;
            }
        }
    }
    return ret;
}
