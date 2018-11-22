#include "history.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "frameworktimer.h"
#include "frameworkutils.h"

using namespace FrameworkLibrary;

History::History(const std::string &history_file):
    _history_filename( history_file )
{
}

History::~History()
{
}

/*void History::_splitTime(uint64_t t,
                         uint32_t &w,
                         uint32_t &d,
                         uint32_t &h,
                         uint32_t &m)
{
    m = (t / 60) % 60; // min 0..59
    h = (t / (60*60)) % 24; // hour 0..23
    // Normalize start day: for EPOC is thursday, for us it's monday.
    // what we do is to "add" three days to the time, so that:
    // - EPOC day0 becomes a monday instead of a thursday
    // - EPOC day3 becomes a thursday, correctly.
    // Note that this will also shift WEEKS! Now the first EPOC week
    // will be 1 and not 0. This means that the "w" below is shifted by one...
    // but we don't care for that since we always work for "difference" between
    // week numbers.
    t += 60*60*24*3; // Now the day-0 for EPOC=3 will be +3days
    d = (t / (60*60*24)) % 7; // Days are 0..6 (mon-sun)
    w = static_cast<uint32_t>((t / (60*60*24*7))); // weeks are not bounded..
}*/

void History::initialize(float &ext_temp,
                         float &ext_humidity)
{
    FILE* read_file = fopen( _history_filename.c_str(), "rb" );
    if ( read_file != nullptr )
    {
        fseek( read_file, 0, SEEK_END );
        long int flen = ftell(read_file);
        // Read LAST ext data:
        fseek( read_file, flen-HistoryItem::getSize(), SEEK_SET );
        HistoryItem last_history(read_file);
        ext_temp = last_history.getExtTemp();
        ext_humidity = last_history.getExtHumidity();
        fclose(read_file);
    }
}

bool History::update(float last_temp,
                     float last_humidity,
                     float last_ext_temp,
                     float last_ext_humidity)
{
    bool ret = true;
    HistoryItem new_item( FrameworkTimer::getTimeEpoc(), last_temp, last_humidity, last_ext_temp, last_ext_humidity );
    FILE* history_file = fopen( _history_filename.c_str(), "ab" );
    if ( history_file != nullptr )
    {
        new_item.write( history_file );
        fclose(history_file);
    }
    else
        ret = false;
    return ret;
}
