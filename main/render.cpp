#include "render.h"
#include <esp_log.h>
#include <lvgl.h>
#include "display_device.h"
#include <debounced_button.hpp>

#define TAG "GHOME_RENDER"

#define LCD_WIDTH 135
#define LCD_HEIGHT 240

#define DRAW_BUF_SIZE (LCD_WIDTH * LCD_HEIGHT * (LV_COLOR_DEPTH / 8))
DRAM_ATTR WORD_ALIGNED_ATTR uint32_t draw_buf[DRAW_BUF_SIZE / 4];

struct devices
{
    DisplayDevice lcd{LCD_WIDTH, LCD_HEIGHT};
    lv_display_t *display{nullptr};
    lv_indev_t *input{nullptr};
};

devices dev{};

struct components
{
    lv_obj_t *tab;

    uint tab_index = 0;
    void changeTab(uint index)
    {
        tab_index = index;
        lv_tabview_set_active(tab, tab_index, LV_ANIM_ON);
    }
};

components ui{};

FORCE_INLINE_ATTR uint32_t lv_tick(void)
{
    return esp_timer_get_time() / 1000;
}

FORCE_INLINE_ATTR void lv_disp_flush(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    auto w = (area->x2 - area->x1 + 1);
    auto h = (area->y2 - area->y1 + 1);
    uint32_t size = w * h;

    dev.lcd.startWrite();                            /* Start new TFT transaction */
    dev.lcd.setAddrWindow(area->x1, area->y1, w, h); /* set the working window */
    dev.lcd.writePixels((uint16_t *)px_map, size, true);
    dev.lcd.endWrite(); /* terminate TFT transaction */

    lv_display_flush_ready(display); /* tell lvgl that flushing is done */
}

FORCE_INLINE_ATTR void lv_input_read(lv_indev_t *indev, lv_indev_data_t *data)
{
}

void lv_setup_components()
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

void ghome_render_init()
{
    ESP_LOGI(TAG, "GHOME render init");

    dev.lcd.init();
    dev.lcd.fillScreen(TFT_BLACK);

    ESP_LOGI(TAG, "PANEL_INIT_DONE");

#if 0
    for (int i = 0; i < 10; i++)
    {
        char tmp[50];
        sprintf(tmp, "Hello: %d", i);

        //lcd.drawString(tmp, 0, i * 10);
        if (i % 2 == 0)
            lcd.fillRect(i * 10, i * 10, 10, 10, TFT_WHITE);
        
        lcd.drawRect(i * 10, i * 10, 10, 10, TFT_BLACK);
    }
#endif

    lv_init();
    lv_tick_set_cb(lv_tick);

    dev.display = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    lv_display_set_flush_cb(dev.display, lv_disp_flush);
    ESP_LOGI(TAG, "LVGL_DISPLAY_DONE");

    lv_display_set_buffers(dev.display, draw_buf, NULL, sizeof(draw_buf), LV_DISP_RENDER_MODE_PARTIAL);
    lv_display_set_antialiasing(dev.display, false);
    lv_display_set_color_format(dev.display, LV_COLOR_FORMAT_RGB565);

    ESP_LOGI(TAG, "LVGL_DISPLAY_BUFF_DONE");

    dev.input = lv_indev_create();
    lv_indev_set_type(dev.input, LV_INDEV_TYPE_NONE);
    lv_indev_set_read_cb(dev.input, lv_input_read);
    ESP_LOGI(TAG, "LVGL_INPUT_INIT_DONE");

    ESP_LOGI(TAG, "LVGL_INIT_DONE");

    lv_setup_components();
}

void ghome_render_update()
{
    static DebouncedButton button(GPIO_NUM_0, GPIO_PULLDOWN_DISABLE, GPIO_PULLUP_ENABLE, 100, true);

    button.Step(lv_tick());
    if (button.Pressed())
    {
        ESP_LOGI(TAG, "Button pressed");
        ui.tab_index = (ui.tab_index + 1) % 2;
        ui.changeTab(ui.tab_index);
    }

    lv_timer_handler();
}