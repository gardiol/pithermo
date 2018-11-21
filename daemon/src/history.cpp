#include "history.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "frameworktimer.h"
#include "frameworkutils.h"

using namespace FrameworkLibrary;

History::History(const std::string &history_file,
                 const std::string& exchange_path):
    _now(0),
    _now_min(0),
    _now_hour(0),
    _now_day(0),
    _now_week(0),
    _now_year(0),
    _history_filename( history_file ),
    _exchange_path( exchange_path ),
    _now_filename()
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

void History::_readNow()
{
    _now = FrameworkTimer::getTimeEpoc();
    _now_min = (_now / 60) % 60; // min 0..59
    _now_hour = (_now / (60*60)) % 24; // hour 0..23
    // Normalize start day: for EPOC is thursday, for us it's monday.
    // what we do is to "add" three days to the time, so that:
    // - EPOC day0 becomes a monday instead of a thursday
    // - EPOC day3 becomes a thursday, correctly.
    // Note that this will also shift WEEKS! Now the first EPOC week
    // will be 1 and not 0. This means that the "w" below is shifted by one...
    // but we don't care for that since we always work for "difference" between
    // week numbers.
    uint64_t normalized_now = _now + 60*60*24*3; // Now the day-0 for EPOC=3 will be +3days
    _now_day = (normalized_now / (60*60*24)) % 7; // Days are 0..6 (mon-sun)
    _now_week = static_cast<uint32_t>((normalized_now / (60*60*24*7))); // weeks are not bounded..
    // Ensure folder exist:
    _now_filename = _exchange_path + FrameworkUtils::getDirSep() + "_history";
    if ( ! FrameworkUtils::fileExist( _now_filename, true ) )
        FrameworkUtils::mkDir( _now_filename );
    _now_filename += FrameworkUtils::getDirSep() + FrameworkUtils::tostring( _now_week );
    if ( ! FrameworkUtils::fileExist( _now_filename, true ) )
        FrameworkUtils::mkDir( _now_filename );
    _now_filename += FrameworkUtils::getDirSep() + FrameworkUtils::tostring( _now_day );
    if ( ! FrameworkUtils::fileExist( _now_filename, true ) )
        FrameworkUtils::mkDir( _now_filename );
    _now_filename += FrameworkUtils::getDirSep() + "history_data";
}

void History::initialize(float &ext_temp,
                         float &ext_humidity)
{
    _readNow();
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
    _readNow();
    HistoryItem new_item( _now, last_temp, last_humidity, last_ext_temp, last_ext_humidity );
    FILE* history_file = fopen( _history_filename.c_str(), "ab" );
    if ( history_file != nullptr )
    {
        new_item.write( history_file );
        fclose(history_file);
    }
    else
        ret = false;

    FILE* now_file = fopen( _now_filename.c_str(), "a" );
    if ( now_file != nullptr )
    {
        new_item.writeNow( history_file );
        fclose(now_file);
    }
    else
        ret = false;
    return ret;
}
