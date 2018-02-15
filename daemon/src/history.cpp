#include "history.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "frameworktimer.h"

using namespace FrameworkLibrary;

History::History(const std::string &history_file, const std::string& exchange_path):
    _skip_minutes(0),
    _num_lines(20),
    _points_per_line(60),
    _now(0),
    _now_min(0),
    _now_hour(0),
    _now_day(0),
    _now_week(0),
    _history_filename( history_file ),
    _exchange_path( exchange_path ),
    _mode( 'h' )
{
}

History::~History()
{
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
    if ( len < 1 )
        len = 1;
    if ( len > num_weeks )
        len = num_weeks;
    _num_lines = len;

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
    FILE* read_file = fopen( _history_filename.c_str(), "rb" );
    if ( read_file != NULL )
    {
        fseek( read_file, 0, SEEK_END );
        uint32_t flen = ftell(read_file);
        // The MAXIMUM number of points to read is:
        uint32_t max_points = num_weeks * num_days * num_hours * num_mins;
        // Oldest accepted time:
        uint32_t size = HistoryItem::getSize() * max_points;
        if ( size > flen )
            fseek( read_file, 0, SEEK_SET);
        else
            fseek( read_file, flen - size, SEEK_SET);
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
    setMode( mode );
}

bool History::update(float last_temp, float last_humidity, float last_ext_temp)
{
    _readNow();
    HistoryItem new_item( _now, last_temp, last_humidity, last_ext_temp );
    //              w    d    h
    _history_cache[ 0 ][ _now_day ][ _now_hour ][ _now_min ] = new_item;
    _writeJson();

    FILE* history_file = fopen( _history_filename.c_str(), "ab" );
    if ( history_file != NULL )
    {
        new_item.write( history_file );
        fclose(history_file);
    }
    else
        return false;
    return true;
}

void History::setMode(const std::string &mode)
{
    // default is for hour
    _mode = 'h';

    _points_per_line = 60; // 1h
    _skip_minutes = 1; // Mark 1min
    if ( mode == "d" ) // 24 hours
    {
        _points_per_line = 24*4; // This MUST match to cover the FULL day
        _skip_minutes = 15; // mark 1/4 hour
        _mode = 'd';
    }
    else if ( mode == "w" ) // 7 days
    {
        _points_per_line = 24*7; // This MUST match to cover the FULL week
        _skip_minutes = 60; // Mark 1h
        _mode = 'w';
    }
    _time_strs.clear();
    _time_strs.resize( _num_lines );
    _temp_strs.clear();
    _temp_strs.resize( _num_lines );
    _ext_temp_strs.clear();
    _ext_temp_strs.resize( _num_lines );
    _humi_strs.clear();
    _humi_strs.resize( _num_lines );
    _valid_ptr.clear();
    _valid_ptr.resize( _num_lines );
    for ( uint32_t l = 0; l < _num_lines; ++l )
    {
        _time_strs[l].resize( _points_per_line );
        _temp_strs[l].resize( _points_per_line );
        _ext_temp_strs[l].resize( _points_per_line );
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
    if ( history_json != NULL )
    {
        uint32_t jHour = _now_hour;
        uint32_t jDay = _now_day;
        uint32_t jWweek = 0;
        uint32_t jMin = 0;
        for ( uint32_t l=0; l < _num_lines; l++ )
        {
            switch (_mode)
            {
            case 'h':
                for ( jMin = 0; jMin < num_mins; ++jMin )
                {
                    std::string ti, te, hu, ex_te;
                    _valid_ptr[l][jMin] = _history_cache[jWweek][jDay][jHour][jMin].getStrings( ti, te, hu, ex_te );
                    _time_strs[l][jMin] = ti;
                    _temp_strs[l][jMin] = te;
                    _ext_temp_strs[l][jMin] = ex_te;
                    _humi_strs[l][jMin] = hu;
                }
                if ( jHour > 0 )
                    jHour--;
                else
                {
                    jHour = num_hours-1;
                    if ( jDay > 0 )
                        jDay--;
                    else
                    {
                        jDay = num_days-1;
                        jWweek++; // Since max lines is < num_weeks this will not be an issue of overflow.
                    }
                }
                break;

            case 'd':
                jMin = 0;
                jHour = 0;
                for ( uint32_t p = 0; p < _points_per_line; ++p )
                {
                    std::string ti, te, hu, ex_te;
                    _valid_ptr[l][p] = _history_cache[jWweek][jDay][jHour][jMin].getStrings( ti, te, hu, ex_te );
                    _time_strs[l][p] = ti;
                    _temp_strs[l][p] = te;
                    _ext_temp_strs[l][p] = ex_te;
                    _humi_strs[l][p] = hu;
                    jMin += _skip_minutes;
                    if ( jMin >= num_mins )
                    {
                        jMin = 0;
                        jHour++; // If num_points is calculated correctly, this will not overflow
                    }
                }
                if ( jDay > 0 )
                    jDay--;
                else
                {
                    jDay = num_days-1;
                    jWweek++; // Since max lines is < num_weeks this will not be an issue of overflow.
                }
                break;

            case 'w':
                jMin = 0;
                jHour = 0;
                jDay = 0;
                for ( uint32_t p = 0; p < _points_per_line; ++p )
                {
                    std::string ti, te, ex_te, hu;
                    _valid_ptr[l][p] = _history_cache[jWweek][jDay][jHour][jMin].getStrings( ti, te, hu, ex_te );
                    _time_strs[l][p] = ti;
                    _temp_strs[l][p] = te;
                    _ext_temp_strs[l][p] = ex_te;
                    _humi_strs[l][p] = hu;
                    jMin += _skip_minutes;
                    if ( jMin >= num_mins )
                    {
                        jMin = 0;
                        jHour++;
                        if ( jHour >= num_hours )
                        {
                            jHour = 0;
                            jDay++; // If num_points is correct, this will not overflow
                        }
                    }
                }
                jWweek++; // Since max lines is < num_weeks this will not be an issue of overflow.
                break;
            }
        }

        fwrite("{\"mode\":\"", 9, 1, history_json );
        fwrite( &_mode, 1, 1, history_json );
        fwrite("\",\"len\":", 8,1, history_json);
        fprintf(history_json, "%d", _num_lines );
        fwrite(",\"data\":[", 9, 1, history_json );
        // For each line of history
        for ( uint32_t l = 0; l < _num_lines; ++l )
        {
            if ( l > 0 )
                fwrite( ",", 1, 1, history_json );
            fwrite("{\"time\":[",9, 1, history_json );
            bool not_first = false;
            for ( uint32_t p = 0; p < _points_per_line; ++p )
            {
                if ( _valid_ptr[l][p] )
                {
                    std::string time_str = _time_strs[l][p];
                    if ( not_first )
                        fwrite(",",1,1,history_json);
                    not_first = true;
                    fwrite( time_str.c_str(), time_str.length(), 1, history_json );
                }
            }
            fwrite("],\"temp\":[",10, 1, history_json );
            not_first = false;
            for ( uint32_t p = 0; p < _points_per_line; ++p )
            {
                if ( _valid_ptr[l][p] )
                {
                    std::string temp_str = _temp_strs[l][p];
                    if ( not_first )
                        fwrite(",",1,1,history_json);
                    not_first = true;
                    fwrite( temp_str.c_str(), temp_str.length(), 1, history_json );
                }
            }
            fwrite("],\"ext_temp\":[",14, 1, history_json );
            not_first = false;
            for ( uint32_t p = 0; p < _points_per_line; ++p )
            {
                if ( _valid_ptr[l][p] )
                {
                    std::string temp_str = _ext_temp_strs[l][p];
                    if ( not_first )
                        fwrite(",",1,1,history_json);
                    not_first = true;
                    fwrite( temp_str.c_str(), temp_str.length(), 1, history_json );
                }
            }
            fwrite( "],\"humidity\":[", 14, 1, history_json );
            not_first = false;
            for ( uint32_t p = 0; p < _points_per_line; ++p )
            {
                if ( _valid_ptr[l][p] )
                {
                    std::string humidity_str = _humi_strs[l][p];
                    if ( not_first )
                        fwrite(",",1,1,history_json);
                    not_first = true;
                    fwrite( humidity_str.c_str(), humidity_str.length(), 1, history_json );
                }
            }
            fwrite( "]}", 2, 1, history_json );
        }
        fwrite( "]}", 2, 1, history_json );
        fclose( history_json );
    }
}
