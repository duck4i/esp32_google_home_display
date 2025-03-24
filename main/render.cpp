#include "render.h"
#include <esp_log.h>
#include <lvgl.h>
#include "display_device.h"
#include "lvgl_device.h"
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
    lv_obj_t *tab;
    uint tab_index = 0;
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

    lv_obj_t *lblHello = lv_label_create(tabHome);
    lv_label_set_text(lblHello, "Hello World");
    lv_obj_align(lblHello, LV_ALIGN_CENTER, 0, 0);

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

void ghome_render_init()
{
    ESP_LOGI(TAG, "GHOME render init");

    devices.lcd.init();
    //dev.lcd.fillScreen(TFT_ORANGE);
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

    devices.lvgl.update();
}