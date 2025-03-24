/**
 ******************************************************************************
 * @file        : main.cpp
 * @brief       : Debounced button example
 * @author      : Jacques Supcik <jacques@supcik.net>
 * @date        : 4 April 2024
 ******************************************************************************
 * @copyright   : Copyright (c) 2024 Jacques Supcik
 * @attention   : SPDX-License-Identifier: MIT
 ******************************************************************************
 * @details
 *
 ******************************************************************************
 */

#include "debounced_button.hpp"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" {
void app_main(void);
}

void app_main(void) {
    DebouncedButton button(GPIO_NUM_0, GPIO_PULLDOWN_DISABLE, GPIO_PULLUP_ENABLE, 100, true);

    while (1) {
        uint64_t now = esp_timer_get_time() / 1000;
        button.Step(now);
        if (button.Pressed()) {
            printf("Button pressed\n");
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
