#ifndef SHAREDSTATUS_H
#define SHAREDSTATUS_H

#include <common_defs.h>
#include "sharedmemory.h"

using namespace FrameworkLibrary;

static const SharedMemoryKey SharedStatusKey = 0xA071BCFC;
static const uint32_t SharedStatusMarker = 0xFABAC0F0;

struct SharedStatus
{
    uint32_t marker;
    uint64_t last_update_stamp;
    bool active;
    bool anti_ice_active;
    bool manual_mode;
    bool pellet_on;
    bool pellet_minimum;
    bool pellet_hot;
    bool pellet_flameout;
    bool gas_on;
    bool smart_temp_on;
    float max_temp;
    float min_temp;
    float hysteresis;
    float temp_int;
    float humidity_int;
    float temp_ext;
    float humidity_ext;
    float smart_temp;
    int32_t day;
    int32_t hour;
    int32_t half;
    char program[ 24*2*7 ];
};

#endif // SHAREDSTATUS_H
