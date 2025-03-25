#pragma once
#include <stdint.h>

/*
//  On off
We got a boolean value: for type 0 cluster 6 attribute: 0 -> 1

Endpoint 0x0001's Cluster 0x00000008's Attribute 0x00000000 is 254 **********
*/

enum matter_light_state_t
{
    LIGHT_ON_CHANGE,
    BRIGTHNESS_CHANGE,
};

typedef struct 
{
    matter_light_state_t state_type;
    int16_t value;
} matter_ui_update_msg_t;

typedef void (*matter_event_cb)(const matter_ui_update_msg_t& msg);

void ghome_matter_events_init(void);
void ghome_matter_events_enqueue(matter_ui_update_msg_t msg);
void ghome_matter_events_consume(matter_event_cb cb);

