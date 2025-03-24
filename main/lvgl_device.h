#pragma once
#include <cstdint>
#include <lvgl.h>

class LvglDevice
{
public:
    LvglDevice(uint16_t width, uint16_t height);
    virtual ~LvglDevice();

    void init(lv_display_flush_cb_t flush_cb);
    void deinit(void);

    void update(void);
    static uint32_t getTick(void);

private:
    uint16_t m_width;
    uint16_t m_height;
    lv_display_t *m_display;
    lv_indev_t *m_input;
    uint32_t *m_draw_buffer;
};
