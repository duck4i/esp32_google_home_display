#pragma once
#include <cstdint>
#include "lgfx/v1/LGFXBase.hpp"
#include "lgfx/v1/panel/Panel_ST7789.hpp"
#include "lgfx/v1/platforms/device.hpp"

#define LCD_TYPE_ST7789_SPI         //  1.4" 240x135 ST7789_SPI
// #define LCD_TYPE_ST7789_8BIT     //  2.2" 240x320 ST7789_8BIT    

class DisplayDevice : public lgfx::LGFX_Device
{
public:
    DisplayDevice(uint16_t width, uint16_t height);
    
private:
    lgfx::Panel_ST7789 m_panel;
    
    #ifdef LCD_TYPE_ST7789_SPI
    lgfx::Bus_SPI m_bus;
    #endif

    #ifdef LCD_TYPE_ST7789_8BIT
    lgfx::Bus_Parallel8 m_bus;
    #endif
};
