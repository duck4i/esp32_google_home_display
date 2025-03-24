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
        ESP_LOGI(TAG, "GHOME task is running");
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting GHOME systems");

    xTaskCreate(ghome_task, "ghome_task", 4096, NULL, 5, NULL);
}