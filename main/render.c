#include "render.h"
#include <esp_log.h>
#define TAG "GHOME_RENDER"

#define LCD_WIDTH 135
#define LCD_HEIGHT 240

#define LCD_MOSI 23
#define LCD_SCLK 18
#define LCD_BLK 32
#define LCD_CS 15
#define LCD_DC 2
#define LCD_RST 4

void ghome_render_init()
{
    ESP_LOGI(TAG, "GHOME render init");
}