/**
 ******************************************************************************
 * @file        : debounced_button.hpp
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

#pragma once

#include "driver/gpio.h"

class DebouncedButton {
   public:
    /**
     * @brief Construct a new Debounced Button object
     * @param pin GPIO pin number
     * @param pullDown GPIO pull down configuration
     * @param pullUp GPIO pull up configuration
     * @param debounceTimeMs Debounce time in ms
     * @param invert Invert the button logic (usually true for pull-up and false for pull-down)
     */
    DebouncedButton(gpio_num_t pin,
                    gpio_pulldown_t pullDown = GPIO_PULLDOWN_DISABLE,
                    gpio_pullup_t pullUp = GPIO_PULLUP_ENABLE,
                    uint32_t debounceTimeMs = 100,
                    bool invert = true);
    virtual ~DebouncedButton();

    /**
     * @brief Step the debouncer
     * @param now Current time in ms
     * @details This function must be called periodically to debounce the button and update the
     * internal state
     */
    void Step(int64_t now);

    /**
     * @brief Reset the debouncer
     * @details This function resets the debouncer to its initial state
     */
    void Reset();

    /**
     * @brief Check if the button is pressed
     * @return true if the button is pressed
     * @details button is considered pressed if it is down at the last call to Step and was up at
     * the previous call to Step.
     */
    bool Pressed();

    /**
     * @brief Check if the button is released
     * @return true if the button is released
     * @details button is considered released if it is up at the last call to Step and was down at
     the previous call to Step.
    */
    bool Released();

    /**
     * @brief Check if the button is down
     * @return true if the button was down at the last call to Step
     */
    bool Down();

    /**
     * @brief Check if the button is up
     * @return true if the button was up at the last call to Step
     */
    bool Up();

   private:
    gpio_num_t pin_;
    int debounceTimeMs_;
    bool invert_;
    uint64_t lastChange_;
    int state_;
    int lastState_;
};
