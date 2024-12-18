#include "history.h"

#include <stdio.h>

#include "pithermotimer.h"

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
    HistoryItem new_item( PithermoTimer::getTimeEpoc(), last_temp, last_humidity, last_ext_temp, last_ext_humidity );
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

bool History::fetchInterval(uint64_t from, uint64_t to, std::list<HistoryItem> &items)
{
    bool ret = false;
    items.clear();
    FILE* read_file = fopen( _history_filename.c_str(), "rb" );
    if ( read_file != nullptr )
    {
        bool start_found = false;
        bool end_found = false;
        fseek( read_file, 0, SEEK_END );
        long int file_len = ftell(read_file);
        int64_t item_size = HistoryItem::getSize();
        int64_t total_items = file_len / item_size;
        HistoryItem item;

        int64_t left = 0;
        int64_t right = total_items;
        bool error = false;
        while ( !start_found && !error && (left != (right-1) ) )
        {
            int64_t start_item = (left+right)/2;
            long int cursor = start_item * item_size;
            if ( fseek( read_file, cursor, SEEK_SET ) != -1 )
            {
                item.read( read_file );
                if ( item.isValid() )
                {
                    uint64_t item_time = item.getTime();
                    // This case is actually VERY difficult to happen!
                    if ( item_time == from )
                        start_found = true;
                    else if ( item_time > from )
                        right = start_item;
                    else
                        left = start_item;
                }
                else
                    error = true;
            }
            else
                error = true;
        }

        if ( !error )
        {
        // Let's read until last:
            while ( !end_found && item.isValid() && !feof(read_file) )
            {
                items.push_back( item );
                if ( item.getTime() >= to )
                    end_found = true;
                item.read( read_file );
            }
            if ( items.size() > 1 )
                ret = true;
            fclose(read_file);
        } // error looking for start
    }
    return ret;
}

bool History::calculateStats(uint64_t from, uint64_t to, float &min_t, float &max_t, float &avg_t, float &min_et, float &max_et, float &avg_et)
{
    bool ret = false;
    std::list<HistoryItem> items;
    min_t = min_et = max_t = max_et = avg_t = avg_et = 0;
    if ( fetchInterval( from, to, items ) )
    {
        bool first = true;
        for ( std::list<HistoryItem>::iterator i = items.begin(); i != items.end(); ++i )
        {
            float t = (*i).getTemp(), et = (*i).getExtTemp();
            if ( first || (t < min_t) ) min_t = t;
            if ( first || (t > max_t) ) max_t = t;
            if ( first || (et < min_et) ) min_et = et;
            if ( first || (et > max_et) ) max_et = et;
            if ( first )
            {
                avg_t = t;
                avg_et = et;
                first = false;
                ret = true;
            }
            else
            {
                avg_t = (avg_t + t)/2.0f;
                avg_et = (avg_et + et)/2.0f;
            }
        }
    }
    return ret;
}
