#ifndef SHAREDSTATUS_H
#define SHAREDSTATUS_H

#include <common_defs.h>
#include "sharedmemory.h"

using namespace FrameworkLibrary;

static const SharedMemoryKey SharedStatusKey = 0xA071BCFA;
static const uint32_t SharedStatusMarker = 0xFABAC0F0;
static const uint32_t SharedStatusNumTemplates = 5;
static const uint32_t SharedStatusTemplatesNameSize = 128;

struct SharedStatus
{
    uint32_t marker;
    uint64_t last_update_stamp;
    bool active;
    bool anti_ice_active;
    int32_t current_mode;
    bool pellet_on;
    bool pellet_minimum;
    bool pellet_hot;
    bool pellet_flameout;
    bool gas_on;
    bool smart_temp_on;
    float max_temp;
    float min_temp;
    float hysteresis_max;
    float hysteresis_min;
    float temp_int;
    float humidity_int;
    float temp_ext;
    float humidity_ext;
    float smart_temp;
    int32_t day;
    int32_t hour;
    int32_t half;
    uint64_t manual_off_time;
    float excess_threshold;
    bool excess_temp_reached;
    char program[ 24*2*7 ];
    char templates[ SharedStatusNumTemplates ][ 24*2 ];
    char templates_names[ SharedStatusNumTemplates ][ SharedStatusTemplatesNameSize ];
};

#endif // SHAREDSTATUS_H
