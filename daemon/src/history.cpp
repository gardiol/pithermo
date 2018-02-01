#include "history.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "frameworktimer.h"

using namespace FrameworkLibrary;

History::History(const std::string &history_file, const std::string& exchange_path):
    _skip_minutes(0),
    _num_lines(1),
    _points_per_line(60),
    _now(0),
    _now_min(0),
    _now_hour(0),
    _now_day(0),
    _now_week(0),
    _history_filename( history_file ),
    _exchange_path( exchange_path ),
    _history_file(NULL),
    _mode( 'h' )
{
}

History::~History()
{
    if ( _history_file != NULL )
        fclose(_history_file);
}

void History::_splitTime(uint64_t t, uint32_t &w, uint32_t &d, uint32_t &h, uint32_t &m)
{
    m = (t / 60) % num_mins; // min 0..59
    h = (t / (60*60)) % num_hours; // hour 0..23
    // Normalize start day: for EPOC is thursday, for us it's monday.
    // what we do is to "add" three days to the time, so that:
    // - EPOC day0 becomes a monday instead of a thursday
    // - EPOC day3 becomes a thursday, correctly.
    // Note that this will also shift WEEKS! Now the first EPOC week
    // will be 1 and not 0. This means that the "w" below is shifted by one...
    // but we don't care for that since we always work for "difference" between
    // week numbers.
    t += 60*60*24*3; // Now the day-0 for EPOC=3 will be +3days
    d = (t / (60*60*24)) % num_days; // Days are 0..6 (mon-sun)
    w = (t / (60*60*24*7)); // weeks are not bounded..
}

void History::_readNow()
{
    _now = FrameworkTimer::getTimeEpoc();
    uint32_t old_week = _now_week;
    _splitTime( _now, _now_week, _now_day, _now_hour, _now_min );
    if ( (old_week != 0) && (_now_week != old_week) )
    {
        // Shift all weeks:
        for ( uint32_t w = num_weeks-1; w > 0; --w )
            _history_cache[w] = _history_cache[w-1];
        _history_cache[0].clear();
        _history_cache[0].resize( num_days );
        for ( int d = 0; d < num_days; ++d )
        {
            _history_cache[0][d].resize( num_hours );
            for ( int h = 0; h < num_hours; ++h )
                _history_cache[0][d][h].resize( num_mins );
        }
    }
}

void History::initialize(const std::string &mode, uint32_t len)
{
    _history_cache.clear();
    _history_cache.resize(num_weeks);
    for ( int w = 0; w < num_weeks; ++w )
    {
        _history_cache[w].resize( num_days );
        for ( int d = 0; d < num_days; ++d )
        {
            _history_cache[w][d].resize( num_hours );
            for ( int h = 0; h < num_hours; ++h )
                _history_cache[w][d][h].resize( num_mins );
        }
    }
    _readNow();
    _history_file = fopen( _history_filename.c_str(), "a" );
    FILE* read_file = fopen( _history_filename.c_str(), "r" );
    if ( read_file != NULL )
    {
        fseek( read_file, 0, SEEK_END );
        uint32_t len = ftell(read_file);
        // The MAXIMUM number of points to read is:
        uint32_t max_points = num_weeks * num_days * num_hours * num_mins;
        // Oldest accepted time:
        uint32_t size = HistoryItem::getSize() * max_points;
        if ( size > len )
            fseek( read_file, 0, SEEK_SET);
        else
            fseek( read_file, len - size, SEEK_SET);
        for (uint32_t i = 0; !feof(read_file); ++i )
        {
            HistoryItem item(read_file);
            if ( item.isValid() )
            {
                uint64_t pt_time = item.getTime();
                uint32_t w = 0;
                uint32_t d = 0;
                uint32_t h = 0;
                uint32_t m = 0;
                // Split point time into week/day/hour/min
                _splitTime( pt_time, w, d, h, m );
                // Normalize to our cache:
                w = _now_week - w;
                // Check valid:
                if ( (w < num_weeks) )
                {
                    _history_cache[w][d][h][m] = item;
                }
            }
        }
        fclose(read_file);
    }
    setModeLen( mode, len );
}

void History::update(float last_temp, float last_humidity)
{
    _readNow();
    HistoryItem new_item( _now, last_temp, last_humidity );
    new_item.writeToFile( _history_file );
    //              w    d    h
    _history_cache[ 0 ][ 0 ][ 0 ][ _now_min ] = new_item;
    _writeJson();
}

void History::setModeLen(const std::string &mode, uint32_t len)
{
    // default is for hour
    _mode = 'h';
    _num_lines = len;
    if ( _num_lines == 0 )
        _num_lines = 1;
    if ( _num_lines > 7 ) // 7, because the smallest is DAYS in a week
        _num_lines = 7;

    _points_per_line = 60; // 1h
    _skip_minutes = 1; // Mark 1min
    if ( mode == "d" ) // 24 hours
    {
        _points_per_line = 96;
        _skip_minutes = 15; // mark 1/4 hour
        _mode = 'd';
    }
    else if ( mode == "w" ) // 7 days
    {
        _points_per_line = 24*7;
        _skip_minutes = 60; // Mark 1h
        _mode = 'w';
    }
    _time_strs.clear();
    _time_strs.resize( _num_lines );
    _temp_strs.clear();
    _temp_strs.resize( _num_lines );
    _humi_strs.clear();
    _humi_strs.resize( _num_lines );
    _valid_ptr.clear();
    _valid_ptr.resize( _num_lines );
    for ( uint32_t l = 0; l < _num_lines; ++l )
    {
        _time_strs[l].resize( _points_per_line );
        _temp_strs[l].resize( _points_per_line );
        _humi_strs[l].resize( _points_per_line );
        _valid_ptr[l].resize( _points_per_line );
    }
    _writeJson();
}

void History::_writeJson()
{
    // Format:
    // Array of "lines".
    // Each line has two sets: temp/time and humidity/time
    // Each set has an x and an y.
    // Example: [{temp:[{x:24,y:123456}],humidity:[{x:50,y:123456}]}]
    //
    FILE* history_json = fopen( (_exchange_path+"/_history").c_str(), "w" );
    if ( history_json )
    {
        for ( uint32_t l=0; l < _num_lines; l++ )
        {
            uint32_t m = 0;
            for ( uint32_t p = 0; p < _points_per_line; ++p )
            {
                std::string ti, te, hu;
                switch (_mode)
                {
                case 'h':
                    _valid_ptr[l][p] = _history_cache[0][0][l][m].getStrings( ti, te, hu );
                    break;

                case 'd':
                    _valid_ptr[l][p] = _history_cache[0][l][(m/60)%24][m%60].getStrings( ti, te, hu );
                    break;

                case 'w':
                    _valid_ptr[l][p] = _history_cache[l][(m/(60*24))%7][(m/60)%24][m%60].getStrings( ti, te, hu );
                    break;
                }
                _time_strs[l][p] = ti;
                _temp_strs[l][p] = te;
                _humi_strs[l][p] = hu;
                m += _skip_minutes;
            }
        }

        fwrite("{\"mode\":\"", 9, 1, history_json );
        fwrite( &_mode, 1, 1, history_json );
        fwrite("\",\"len\":", 8,1, history_json);
        fprintf(history_json, "%d", _num_lines );
        fwrite(",\"data\":[", 9, 1, history_json );
        for ( uint32_t l = 0; l < _num_lines; ++l )
        {
            if ( l > 0 )
                fwrite( ",", 1, 1, history_json );
            fwrite("{\"temp\":[",9, 1, history_json );
            bool not_first = false;
            for ( uint32_t p = 0; p < _points_per_line; ++p )
            {
                if ( _valid_ptr[l][p] )
                {
                    std::string temp_str = _temp_strs[l][p];
                    std::string time_str = _time_strs[l][p];
                    if ( not_first )
                        fwrite(",",1,1,history_json);
                    not_first = true;
                    fwrite( "{\"x\":", 5, 1, history_json );
                    fwrite( time_str.c_str(), time_str.length(), 1, history_json );
                    fwrite( ",\"y\":", 5, 1, history_json );
                    fwrite( temp_str.c_str(), temp_str.length(), 1, history_json );
                    fwrite( "}", 1, 1, history_json );
                }
            }
            fwrite( "],\"humidity\":[", 14, 1, history_json );
            not_first = false;
            for ( uint32_t p = 0; p < _points_per_line; ++p )
            {
                if ( _valid_ptr[l][p] )
                {
                    std::string humidity_str = _humi_strs[l][p];
                    std::string time_str = _time_strs[l][p];
                    if ( not_first )
                        fwrite(",",1,1,history_json);
                    not_first = true;
                    fwrite( "{\"x\":", 5, 1, history_json );
                    fwrite( time_str.c_str(), time_str.length(), 1, history_json );
                    fwrite( ",\"y\":", 5, 1, history_json );
                    fwrite( humidity_str.c_str(), humidity_str.length(), 1, history_json );
                    fwrite( "}", 1, 1, history_json );
                }
            }
            fwrite( "]}", 2, 1, history_json );
        }
        fwrite( "]}", 2, 1, history_json );
        fclose( history_json );
    }
}
