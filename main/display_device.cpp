#include "display_device.h"
#include <esp_log.h>

#define TAG "DISPLAY_DEVICE"

#ifdef LCD_TYPE_ST7789_SPI
#define LCD_MOSI 23
#define LCD_SCLK 18
#define LCD_BLK 32
#define LCD_CS 15
#define LCD_DC 2
#define LCD_RST 4
#endif

DisplayDevice::DisplayDevice(uint16_t width, uint16_t height)
{
    ESP_LOGI(TAG, "DisplayDevice constructor");

#if defined(LCD_TYPE_ST7789_SPI)

    //  Configure bus
    auto bcfg = m_bus.config();

    // SPI bus pins
    //bcfg.spi_host = HSPI_HOST;
    bcfg.spi_mode = 0;
    // bcfg.freq_write = 40000000; // 40MHz
    // bcfg.freq_read = 16000000;  // 16MHz
    bcfg.spi_3wire = false;
    bcfg.use_lock = true;
    bcfg.dma_channel = 1;

    // Pin assignment
    bcfg.pin_mosi = LCD_MOSI;
    bcfg.pin_miso = -1; // Not used
    bcfg.pin_sclk = LCD_SCLK;
    bcfg.pin_dc = LCD_DC;

    m_bus.config(bcfg);
    m_panel.setBus(&m_bus);

    //  Configure the panel
    auto cfg = m_panel.config();

    cfg.pin_cs = LCD_CS;
    cfg.pin_rst = LCD_RST;
    cfg.pin_busy = -1;

    cfg.panel_width = width;
    cfg.panel_height = height;
    cfg.offset_x = 52;
    cfg.offset_y = 40;
    // cfg.offset_rotation = 1;
    cfg.readable = false;
    cfg.invert = true;
    cfg.rgb_order = false;
    cfg.dlen_16bit = false;
    cfg.bus_shared = false;

    m_panel.config(cfg);
    setPanel(&m_panel);

#elif defined(LCD_TYPE_ST7789_8BIT)
    auto bcfg = m_bus.config();
    bcfg.pin_wr = 4;
    bcfg.pin_rd = 2;
    bcfg.pin_rs = 16;

    bcfg.pin_d0 = 15;
    bcfg.pin_d1 = 13;
    bcfg.pin_d2 = 12;
    bcfg.pin_d3 = 14;
    bcfg.pin_d4 = 27;
    bcfg.pin_d5 = 25;
    bcfg.pin_d6 = 33;
    bcfg.pin_d7 = 32;

    m_bus.config(bcfg);
    m_panel.setBus(&m_bus);

    auto cfg = m_panel.config();

    cfg.pin_cs = 17;
    cfg.pin_rst = -1;
    cfg.pin_busy = -1;

    cfg.panel_width = width;
    cfg.panel_height = height;
    cfg.offset_x = 0;
    cfg.offset_y = 0;
    cfg.offset_rotation = 0;
    // cfg.dummy_read_pixel = 8;
    // cfg.dummy_read_bits = 1;
    cfg.readable = false;
    cfg.invert = false;
    cfg.rgb_order = false;
    cfg.dlen_16bit = false;
    cfg.bus_shared = false;

    m_panel.config(cfg);
    setPanel(&m_panel);

#else
#error "No panel selected"
#endif
}