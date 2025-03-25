#pragma once
#include <stdint.h>

/*
//  On off
We got a boolean value: for type 0 cluster 6 attribute: 0 -> 1

// Brigthness
Endpoint 0x0001's Cluster 0x00000008's Attribute 0x00000000 is 254 **********

I (257066) esp_matter_attribute: ********** R : Endpoint 0x0001's Cluster 0x00000300's Attribute 0x00000002 is 1 **********
I (257076) esp_matter_attribute: ********** W : Endpoint 0x0001's Cluster 0x00000300's Attribute 0x00000002 is 0 **********
I (257086) GHOME_MATTER: [CBC] Attribute update callback: type: 0, cluster: 768, attribute: 2
I (257096) esp_matter_attribute: ********** R : Endpoint 0x0001's Cluster 0x00000300's Attribute 0x00000007 is 435 **********
I (257106) esp_matter_attribute: ********** W : Endpoint 0x0001's Cluster 0x00000300's Attribute 0x00000007 is 370 **********
I (257126) GHOME_MATTER: [CBC] Attribute update callback: type: 0, cluster: 768, attribute: 7
I (257146) chip[ZCL]: Color Temperature 370
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

