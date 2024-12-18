
#include <stdint.h>
#include <list>
#include "../src/pithermoutils.h"
#include "../src/history.h"
#include "../src/logger.h"

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

    if ( PithermoUtils::fileExist( history_file ) &&
         PithermoUtils::fileExist( events_file ) )
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
                uint32_t skip_items = PithermoUtils_max<uint32_t>( n_items / n_samples, 1);
                uint32_t n_item = 0;
                std::list<LogItem>::iterator e = events_items.begin();
                bool pellet_on = false;
                bool pellet_off = false;
                bool gas_on = false;
                bool gas_off = false;
                float min_t = 0, max_t = 0, avg_t = 0,
                        min_et = 0, max_et = 0, avg_et = 0,
                        min_h = 0, max_h = 0, avg_h = 0,
                        min_eh = 0, max_eh = 0, avg_eh = 0;
                bool first = true;
                for ( std::list<HistoryItem>::iterator i = history_items.begin(); i != history_items.end(); ++i )
                {
                    bool skip_events = false;
                    uint64_t history_time = (*i).getTime();
                    float temp = (*i).getTemp();
                    float humi = (*i).getHumidity();
                    float temp_e = (*i).getExtTemp();
                    float humi_e = (*i).getExtHumidity();
                    if ( first || ( temp < min_t ) )
                        min_t = temp;
                    if ( first || ( temp_e < min_et ) )
                        min_et = temp_e;
                    if ( first || ( humi < min_h ) )
                        min_h = humi;
                    if ( first || ( humi_e < min_eh ) )
                        min_eh = humi_e;
                    if ( first || ( temp > max_t ) )
                        max_t = temp;
                    if ( first || ( temp_e > max_et ) )
                        max_et = temp_e;
                    if ( first || ( humi > max_h ) )
                        max_h = humi;
                    if ( first || ( humi_e > max_eh ) )
                        max_eh = humi_e;
                    avg_t = first ? temp : (avg_t+temp)/2.0f;
                    avg_et = first ? temp_e : (avg_et+temp_e)/2.0f;
                    avg_h = first ? humi : (avg_h+humi)/2.0f;
                    avg_eh = first ? humi_e : (avg_eh+humi_e)/2.0f;

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
                               static_cast<double>(temp),
                               static_cast<double>(humi),
                               static_cast<double>(temp_e),
                               static_cast<double>(humi_e),
                               pellet_on ? 1:0,
                               pellet_off ? 1:0,
                               gas_on ? 1:0,
                               gas_off ? 1:0);
                        gas_off = gas_on = pellet_off = pellet_on = false;
                    }
                    n_item++;
                    first = false;
                }
                printf("%f %f %f %f %f %f %f %f %f %f %f %f",
                       static_cast<double>(min_t), static_cast<double>(max_t), static_cast<double>(avg_t),
                       static_cast<double>(min_h), static_cast<double>(max_h), static_cast<double>(avg_h),
                       static_cast<double>(min_et), static_cast<double>(max_et), static_cast<double>(avg_et),
                       static_cast<double>(min_eh), static_cast<double>(max_eh), static_cast<double>(avg_eh) );

                ret = 0;
            }
        }
    }
    return ret;
}
