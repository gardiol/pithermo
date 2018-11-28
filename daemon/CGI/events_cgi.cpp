
#include <stdint.h>
#include <list>
#include "../framework/frameworkutils.h"
#include "../framework/frameworktimer.h"
#include "../src/logger.h"
#include "../src/logitem.h"

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
    std::string events_file = "../events";
    int ret = 255;
    printf("Content-type: text/plain\n\n");

    if ( FrameworkUtils::fileExist( events_file ) )
    {
        if ( parse_POST() )
        {
            Logger logger( events_file );
            std::list<LogItem> items;

            if ( logger.fetchInterval( from_time, to_time, items ) )
            {
                uint32_t n_item = 0;
                for ( std::list<LogItem>::iterator i = items.begin();
                      (i != items.end()) && (n_item < n_samples); ++i )
                {
                    (*i).writeText( stdout );
                    n_item++;
                }
                if ( n_item < items.size() )
                    printf("+");
                ret = 0;
            }
        }
    }
    return ret;
}