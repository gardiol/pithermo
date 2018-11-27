
#include <stdint.h>
#include <list>
#include "../framework/frameworkutils.h"
#include "../framework/frameworktimer.h"
#include "../src/history.h"

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
    int ret = 255;
    printf("Content-type: text/plain\n\n");

    if ( FrameworkUtils::fileExist( history_file ) )
    {
        if ( parse_POST() )
        {
            History history( history_file );
            std::list<HistoryItem> items;

            if ( history.fetchInterval( from_time, to_time, items ) )
            {
                uint32_t n_items = static_cast<uint32_t>(items.size());
                uint32_t skip_items = FrameworkUtils_max<uint32_t>( n_items / n_samples, 1);
                uint32_t n_item = 0;
                for ( std::list<HistoryItem>::iterator i = items.begin(); i != items.end(); ++i )
                {
                    if ( (n_item % skip_items) == 0 )
                        (*i).writeText( stdout );
                    n_item++;
                }
                ret = 0;
            }
        }
    }
    return ret;
}
