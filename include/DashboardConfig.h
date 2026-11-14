#pragma once

#include <stdint.h>

#ifndef DASH_VERSION
#define DASH_VERSION "0.3.2"
#endif

#ifndef DASH_ROTATION
#define DASH_ROTATION 1
#endif

#if !defined(DASH_BOARD_WAVESHARE_7)
#if defined(DASH_TOUCH_CAPACITIVE) == defined(DASH_TOUCH_RESISTIVE)
#error "Select exactly one touch type"
#endif
#endif

#if DASH_ROTATION != 0 && DASH_ROTATION != 1
#error "DASH_ROTATION must be 0 or 1"
#endif

namespace dash
{
#if defined(DASH_BOARD_WAVESHARE_7)
    constexpr uint16_t iNativeWidth = 800;
    constexpr uint16_t iNativeHeight = 480;
    constexpr uint16_t iWidth = DASH_ROTATION ? 480 : 800;
    constexpr uint16_t iHeight = DASH_ROTATION ? 800 : 480;
    constexpr uint16_t iDrawBufferLines = 20;
    constexpr const char* sBoardName = "Waveshare ESP32-S3-Touch-LCD-7 Rev1.2";
    constexpr const char* sTouchName = "GT911";
    constexpr const char* sOrientationName = DASH_ROTATION ? "Portrait" : "Paysage";
#else
    constexpr int iLcdMosi = 13;
    constexpr int iLcdMiso = 12;
    constexpr int iLcdSclk = 14;
    constexpr int iLcdCs = 15;
    constexpr int iLcdDc = 2;
    constexpr int iLcdBl = 27;
    constexpr int iLcdRst = -1;

    constexpr int iTouchSda = 33;
    constexpr int iTouchScl = 32;
    constexpr int iTouchRst = 25;
    constexpr int iTouchInt = -1; // Poll GT911: IRQ is unconnected on some revisions.
    constexpr int iTouchCs = 33;

    constexpr uint16_t iNativeWidth = 320;
    constexpr uint16_t iNativeHeight = 480;
    constexpr uint16_t iWidth = DASH_ROTATION ? iNativeHeight : iNativeWidth;
    constexpr uint16_t iHeight = DASH_ROTATION ? iNativeWidth : iNativeHeight;
    constexpr uint16_t iDrawBufferLines = 10;
    // ST7796 SPI parameters cross-checked against user's working TFT_eSPI setup.
    // Override per environment without editing the display driver.
#ifndef DASH_LCD_SPI_WRITE_HZ
#define DASH_LCD_SPI_WRITE_HZ 27000000UL
#endif
#ifndef DASH_LCD_SPI_READ_HZ
#define DASH_LCD_SPI_READ_HZ 16000000UL
#endif
    constexpr uint32_t ulLcdSpiHz = DASH_LCD_SPI_WRITE_HZ;
    constexpr uint32_t ulLcdSpiReadHz = DASH_LCD_SPI_READ_HZ;

#if defined(DASH_TOUCH_CAPACITIVE)
    constexpr const char* sBoardName = "ESP32-3248S035C";
    constexpr const char* sTouchName = "GT911";
#else
    constexpr const char* sBoardName = "ESP32-3248S035R";
    constexpr const char* sTouchName = "XPT2046";
#endif

    constexpr const char* sOrientationName = DASH_ROTATION ? "Paysage" : "Portrait";
#endif
}
