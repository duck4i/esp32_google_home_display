#include "render.h"
#include <esp_log.h>
#include "lgfx/v1/LGFXBase.hpp"
#include "lgfx/v1/panel/Panel_ST7789.hpp"
#include "lgfx/v1/platforms/device.hpp"

#define TAG "GHOME_RENDER"

#define LCD_WIDTH 135
#define LCD_HEIGHT 240

#define LCD_MOSI 23
#define LCD_SCLK 18
#define LCD_BLK 32
#define LCD_CS 15
#define LCD_DC 2
#define LCD_RST 4

class Device : public lgfx::LGFX_Device
{
private:
    lgfx::Panel_ST7789 m_panel;
    lgfx::Bus_SPI m_bus;

public:
    Device()
    {
        //  Configure bus
        auto bcfg = m_bus.config();

        // SPI bus pins
        bcfg.spi_host = HSPI_HOST;
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

        cfg.panel_width = LCD_WIDTH;
        cfg.panel_height = LCD_HEIGHT;
        cfg.offset_x = 52;
        cfg.offset_y = 40;
        //cfg.offset_rotation = 1;
        cfg.readable = false;
        cfg.invert = true;
        cfg.rgb_order = false;
        cfg.dlen_16bit = false;
        cfg.bus_shared = false;

        m_panel.config(cfg);

        setPanel(&m_panel);
    }
};

Device lcd = Device();

void ghome_render_init()
{
    ESP_LOGI(TAG, "GHOME render init");

    lcd.init();
    lcd.fillScreen(TFT_ORANGE);

    lcd.setTextSize(1);
    lcd.setTextColor(TFT_YELLOW);
    lcd.setFont(&lgfx::fonts::FreeMono9pt7b);
    lcd.setRotation(1);

    for (int i = 0; i < 10; i++)
    {
        char tmp[50];
        sprintf(tmp, "Hello: %d", i);

        //lcd.drawString(tmp, 0, i * 10);
        if (i % 2 == 0)
            lcd.fillRect(i * 10, i * 10, 10, 10, TFT_WHITE);
        
        lcd.drawRect(i * 10, i * 10, 10, 10, TFT_BLACK);
    }
}