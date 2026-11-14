#pragma once

#if defined(DASH_DISPLAY_TFT_ESPI)
#include <TFT_eSPI.h>
#include <Wire.h>
#if defined(DASH_BOARD_CYD_28) || defined(DASH_BOARD_CYD_24)
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#endif
#elif !defined(DASH_BOARD_WAVESHARE_7)
#include <LovyanGFX.hpp>
#endif
#include "DashboardConfig.h"

// Display backend: ST7796 SPI (3.5-inch) or ST7262 RGB (Waveshare 7-inch).
#if defined(DASH_BOARD_WAVESHARE_7)
// Waveshare_ST7262_LVGL initializes the RGB LCD, GT911, CH422G and LVGL.
// The 7-inch panel is intentionally NOT driven via LovyanGFX.
class DashboardDisplay
{
public:
    bool initializeBoard();
};
#elif defined(DASH_DISPLAY_TFT_ESPI)
// Based on the proven TFT_eSPI ST7796 configuration for 3248S035C.
class DashboardDisplay : public TFT_eSPI
{
public:
    void setBrightness(uint8_t iLevel);
    bool getTouch(uint16_t* pX, uint16_t* pY);
    // Diagnostic GT911: expose status and all bytes without clamping.
    // Returns true only for a valid touch report.
    bool getTouchRaw(uint16_t* pX, uint16_t* pY, uint8_t* pStatus, uint8_t* pData);
    void initializeTouch();
    bool isTouchReady() const;

private:
    bool bTouchReady = false;
#if defined(DASH_BOARD_CYD_28) || defined(DASH_BOARD_CYD_24)
    SPIClass oTouchBus = SPIClass(HSPI);
    XPT2046_Touchscreen oTouch = XPT2046_Touchscreen(33);
#endif
};
#else
class DashboardDisplay : public lgfx::LGFX_Device
{
public:
    DashboardDisplay();

private:
    lgfx::Panel_ST7796 oPanel;
    lgfx::Bus_SPI oBus;
    lgfx::Light_PWM oLight;

#if defined(DASH_TOUCH_CAPACITIVE)
    lgfx::Touch_GT911 oTouch;
#else
    lgfx::Touch_XPT2046 oTouch;
#endif
};
#endif
