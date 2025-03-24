#include "lvgl_device.h"
#include <esp_timer.h>
#include <esp_log.h>
#include <esp_heap_caps.h>

#define TAG "LVGL_DEVICE"

FORCE_INLINE_ATTR void lv_input_read(lv_indev_t *indev, lv_indev_data_t *data)
{
}

LvglDevice::LvglDevice(uint16_t width, uint16_t height) : m_width(width), m_height(height), m_display(nullptr), m_input(nullptr), m_draw_buffer(nullptr)
{
}

LvglDevice::~LvglDevice()
{
    if (m_draw_buffer)
    {
        heap_caps_free(m_draw_buffer);
        m_draw_buffer = nullptr;
    }
    if (m_input)
    {
        lv_indev_delete(m_input);
        m_input = nullptr;
    }
    if (m_display)
    {
        lv_display_delete(m_display);
        m_display = nullptr;
    }
}

void LvglDevice::init(lv_display_flush_cb_t flush_cb)
{
    lv_init();
    lv_tick_set_cb(getTick);

    m_display = lv_display_create(m_width, m_height);
    lv_display_set_flush_cb(m_display, flush_cb);
    ESP_LOGI(TAG, "LVGL_DISPLAY_DONE");

    size_t size = m_width * m_height * (LV_COLOR_DEPTH / 8);
    m_draw_buffer = (uint32_t *)heap_caps_aligned_alloc(4, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    if (m_draw_buffer == nullptr)
    {
        ESP_LOGE(TAG, "Failed to allocate draw buffer");
        return;
    }

    lv_display_set_buffers(m_display, m_draw_buffer, NULL, size, LV_DISP_RENDER_MODE_PARTIAL);
    lv_display_set_antialiasing(m_display, false);
    lv_display_set_color_format(m_display, LV_COLOR_FORMAT_RGB565);
    ESP_LOGI(TAG, "LVGL_DISPLAY_BUFF_DONE");

    m_input = lv_indev_create();
    lv_indev_set_type(m_input, LV_INDEV_TYPE_NONE);
    lv_indev_set_read_cb(m_input, lv_input_read);

    ESP_LOGI(TAG, "LVGL_INIT_DONE");
}

void LvglDevice::deinit()
{
    lv_deinit();
}

uint32_t LvglDevice::getTick(void)
{
    return esp_timer_get_time() / 1000;
}

void LvglDevice::update()
{
    lv_timer_handler();
}