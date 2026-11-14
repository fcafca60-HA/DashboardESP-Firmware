// Standalone 3.5-inch ST7796 bus diagnostic. No LVGL, network or JSON.
// Match the known-good TFT_eSPI GPIO map and SPI timings.
#if defined(DASH_LCD_DIAGNOSTIC_035) && !defined(DASH_BOARD_WAVESHARE_7)
#include <Arduino.h>
#include "DashboardConfig.h"
#include "DashboardDisplay.h"

namespace
{
    DashboardDisplay oScreen;
}

void setup()
{
    Serial.begin(115200);
    delay(300);
    Serial.printf("[LCD TEST] %s / %s / ST7796 TFT_eSPI 27MHz / %u x %u\n",
                  dash::sBoardName, dash::sOrientationName, dash::iWidth, dash::iHeight);
    oScreen.init();
#if defined(DASH_DISPLAY_TFT_ESPI)
    oScreen.setSwapBytes(true);
#endif
    oScreen.setRotation(DASH_ROTATION);
    oScreen.setBrightness(220);
    const uint16_t aColors[6] = {
        TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE, TFT_BLACK, TFT_YELLOW
    };
    const int32_t iBarHeight = oScreen.height() / 6;
    for (int iIndex = 0; iIndex < 6; ++iIndex)
    {
        const int32_t iY = iIndex * iBarHeight;
        const int32_t iHeight = (iIndex == 5) ? (oScreen.height() - iY) : iBarHeight;
        oScreen.fillRect(0, iY, oScreen.width(), iHeight, aColors[iIndex]);
    }
    oScreen.setTextSize(2);
    oScreen.setTextColor(TFT_BLACK, TFT_WHITE);
    oScreen.setCursor(8, oScreen.height() / 2);
    oScreen.print("ST7796 TFT_eSPI 27MHz");
    Serial.println("[LCD TEST] Six solid bands expected. No WiFi/JSON in this profile.");
}

void loop()
{
    delay(1000);
}
#endif
