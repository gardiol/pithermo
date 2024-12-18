#ifndef SHAREDSTATUS_H
#define SHAREDSTATUS_H

#include "sharedmemory.h"

static const SharedMemoryKey SharedStatusKey = 0xA071BCFA;
static const uint32_t SharedStatusMarker = 0xFABAC0F0;
static const uint32_t SharedStatusNumTemplates = 5;
static const uint32_t SharedStatusTemplatesNameSize = 128;
static const uint32_t SharedStatusCommandQueueSize = 50;
static const uint32_t SharedStatusProgramDaySize = 24*2;
static const uint32_t SharedStatusProgramSize = SharedStatusProgramDaySize * 7;

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
    char program[ SharedStatusProgramSize ];
    char templates[ SharedStatusNumTemplates ][ SharedStatusProgramDaySize ];
    char templates_names[ SharedStatusNumTemplates ][ SharedStatusTemplatesNameSize ];

    struct {
        uint32_t command;
        double payload;
        char payload_string[ SharedStatusProgramSize ];
    } command_queue[ SharedStatusCommandQueueSize ];
    uint32_t command_queue_write_ptr;
    int32_t command_queue_write_busy;
    uint32_t command_queue_read_ptr;

};

#endif // SHAREDSTATUS_H
