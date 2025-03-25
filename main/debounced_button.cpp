/**
 ******************************************************************************
 * @file        : debounced_button.cpp
 * @brief       : Debounced button driver
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

#include "esp_err.h"
#include "esp_log.h"

static const char* kTag = "DebouncedButton";

DebouncedButton::DebouncedButton(gpio_num_t pin,
                                 gpio_pulldown_t pullDown,
                                 gpio_pullup_t pullUp,
                                 uint32_t debounceTimeMs,
                                 bool invert)
    : pin_(pin), debounceTimeMs_(debounceTimeMs), invert_(invert) {
    Reset();
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << pin_;
    io_conf.pull_down_en = pullDown;
    io_conf.pull_up_en = pullUp;
    ESP_ERROR_CHECK(gpio_config(&io_conf));
}

DebouncedButton::~DebouncedButton() {}

void DebouncedButton::Reset() {
    if (invert_) {
        state_ = 1;
        lastState_ = 1;
    } else {
        state_ = 0;
        lastState_ = 0;
    }
    lastChange_ = 0;
}

void DebouncedButton::Step(int64_t now) {
    lastState_ = state_;
    if (now - lastChange_ > debounceTimeMs_) {
        int l = gpio_get_level(pin_);
        if (l != state_) {
            ESP_LOGI(kTag, "Button %d: %d", pin_, l);
            lastChange_ = now;
            state_ = l;
        }
    }
}

bool DebouncedButton::Pressed() {
    if (invert_) {
        return state_ == 0 && lastState_ == 1;
    } else {
        return state_ == 1 && lastState_ == 0;
    }
}

bool DebouncedButton::Released() {
    if (invert_) {
        return state_ == 1 && lastState_ == 0;
    } else {
        return state_ == 0 && lastState_ == 1;
    }
}

bool DebouncedButton::Down() {
    if (invert_) {
        return state_ == 0;
    } else {
        return state_ == 1;
    }
}

bool DebouncedButton::Up() {
    if (invert_) {
        return state_ == 1;
    } else {
        return state_ == 0;
    }
}
