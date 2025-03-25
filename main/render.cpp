#include "render.h"
#include <esp_log.h>
#include <lvgl.h>
#include "display_device.h"
#include "lvgl_device.h"
#include "matter_callback.h"
#include <debounced_button.hpp>

#define TAG "GHOME_RENDER"

#define LCD_WIDTH 135
#define LCD_HEIGHT 240

struct devices_t
{
    DisplayDevice lcd{LCD_WIDTH, LCD_HEIGHT};
    LvglDevice lvgl{LCD_WIDTH, LCD_HEIGHT};
};
devices_t devices{};

struct components_t
{
    uint tab_index{0};
    bool led_on{true};
    int16_t brightness{255};
    lv_obj_t *tab{nullptr};
    lv_obj_t *color_pin{nullptr};
    lv_obj_t *brightness_arc{nullptr};
};
components_t ui{};

void ui_set_active_tab(uint index)
{
    ui.tab_index = index;
    lv_tabview_set_active(ui.tab, index, LV_ANIM_ON);
}

void ui_toggle_active_tab()
{
    uint index = (ui.tab_index + 1) % 2;
    ui_set_active_tab(index);
}

void ui_set_pin_state(bool state)
{
    ui.led_on = state;
    if (state)
    {
        lv_led_on(ui.color_pin);
        lv_led_set_brightness(ui.color_pin, ui.brightness);
    }
    else
    {
        lv_led_off(ui.color_pin);
        lv_led_set_brightness(ui.color_pin, 0);
    }
}

void ui_animate_brightness(uint16_t value, uint32_t duration = 125)
{
    static lv_anim_t *brightness_anim = nullptr;
    if (brightness_anim != nullptr)
    {
        lv_anim_delete(brightness_anim, nullptr);
    }
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui.brightness_arc);
    lv_anim_set_values(&a, lv_arc_get_value(ui.brightness_arc), value);
    lv_anim_set_time(&a, duration); // Animation duration in milliseconds
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_arc_set_value);
    brightness_anim = lv_anim_start(&a);
}

void ui_update_brightness(uint16_t value)
{
    uint16_t v = ui.led_on ? value : 0;
    ui.brightness = v;
    lv_led_set_brightness(ui.color_pin, v);
    ui_animate_brightness(v);
}

void ui_setup_components()
{
    lv_obj_t *scr = lv_scr_act();

    lv_obj_t *tab = lv_tabview_create(scr);
    lv_tabview_set_tab_bar_position(tab, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(tab, 32);
    ui.tab = tab;
    ui.tab_index = 0;

    lv_obj_t *tabHome = lv_tabview_add_tab(tab, "Home");
    lv_obj_t *tabSettings = lv_tabview_add_tab(tab, "Settings");

    ui.color_pin = lv_led_create(tabHome);
    lv_led_set_color(ui.color_pin, lv_color_make(0, 255, 0));
    lv_obj_set_size(ui.color_pin, 48, 48);
    lv_obj_center(ui.color_pin);

    if (ui.led_on)
        lv_led_on(ui.color_pin);
    else
        lv_led_off(ui.color_pin);

    ui.brightness_arc = lv_arc_create(tabHome);
    lv_arc_set_range(ui.brightness_arc, 0, 255);
    lv_obj_center(ui.brightness_arc);
    ui_animate_brightness(ui.brightness, 1000);

    lv_obj_t *lblSettings = lv_label_create(tabSettings);
    lv_label_set_text(lblSettings, "Settings");
    lv_obj_align(lblSettings, LV_ALIGN_CENTER, 0, 0);
}

FORCE_INLINE_ATTR void render_flush_display(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    auto w = (area->x2 - area->x1 + 1);
    auto h = (area->y2 - area->y1 + 1);
    uint32_t size = w * h;

    devices.lcd.startWrite();                            /* Start new TFT transaction */
    devices.lcd.setAddrWindow(area->x1, area->y1, w, h); /* set the working window */
    devices.lcd.writePixels((uint16_t *)px_map, size, true);
    devices.lcd.endWrite(); /* terminate TFT transaction */

    lv_display_flush_ready(display); /* tell lvgl that flushing is done */
}

/*
Turn on:
W (184666) GHOME_RENDER: Matter event: 0 Value: 1
W (184916) GHOME_RENDER: Matter event: 1 Value: 1
W (184916) GHOME_RENDER: Matter event: 1 Value: 135

Turn off:
W (87996) GHOME_RENDER: Matter event: 1 Value: 1
W (88086) GHOME_RENDER: Matter event: 0 Value: 0
W (88156) GHOME_RENDER: Matter event: 1 Value: 254
*/

void matter_event_callback(const matter_ui_update_msg_t &msg)
{
    ESP_LOGW(TAG, "Matter event: %d Value: %d", msg.state_type, msg.value);
    switch (msg.state_type)
    {
    case LIGHT_ON_CHANGE:
        ui_set_pin_state(msg.value == true);
        break;
    case BRIGTHNESS_CHANGE:
        ui_update_brightness(msg.value);
        break;
    default:
        break;
    };
}

void ghome_render_init()
{
    ESP_LOGI(TAG, "GHOME render init");

    devices.lcd.init();
    // dev.lcd.fillScreen(TFT_ORANGE);
    ESP_LOGI(TAG, "PANEL_INIT_DONE");

    devices.lvgl.init(render_flush_display);
    ESP_LOGI(TAG, "LGLV_INIT_DONE");

    ui_setup_components();
}

void ghome_render_update()
{
    static DebouncedButton button(GPIO_NUM_0, GPIO_PULLDOWN_DISABLE, GPIO_PULLUP_ENABLE, 100, true);

    button.Step(devices.lvgl.getTick());
    if (button.Pressed())
    {
        ESP_LOGI(TAG, "Button pressed");
        ui_toggle_active_tab();
    }

    ghome_matter_events_consume(matter_event_callback);
    devices.lvgl.update();
}