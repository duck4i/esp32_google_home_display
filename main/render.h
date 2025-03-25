#pragma once
#include <cstdint>

typedef struct { 
    int16_t tab_index;
    int16_t brightness;
    bool led_on;
} render_ui_state_t;

void ghome_render_init();
void ghome_render_update();