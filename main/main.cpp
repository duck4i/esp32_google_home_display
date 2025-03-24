#include <stdio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "render.h"
#include "matter.h"

#define TAG "GHOME"

void render_task(void *pvParameter)
{
    ESP_LOGI(TAG, "GHOME task started.");

    ghome_render_init();

    while (1)
    {
        ghome_render_update();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void matter_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Matter task started.");
    
    ghome_matter_init();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting GHOME systems");

    xTaskCreate(render_task, "ghome_task", 9048, NULL, 5, NULL);
    xTaskCreate(matter_task, "matter_task", 4096, NULL, 5, NULL);
}