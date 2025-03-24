#include <stdio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "render.h"

#define TAG "GHOME"

void ghome_task(void *pvParameter)
{
    ESP_LOGI(TAG, "GHOME task started.");

    ghome_render_init();

    while (1)
    {
        ghome_render_update();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting GHOME systems");

    xTaskCreate(ghome_task, "ghome_task", 9048, NULL, 5, NULL);
}