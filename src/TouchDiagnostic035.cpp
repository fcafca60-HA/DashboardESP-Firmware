// ESP32-3248S035C GT911 RAW register diagnostic V0.3.15.
// Preserve proven TFT_eSPI ST7796 driver; do not depend on LVGL or WiFi.
#if defined(DASH_TOUCH_DIAGNOSTIC_035) && defined(DASH_DISPLAY_TFT_ESPI)
#include <Arduino.h>
#include "DashboardDisplay.h"

namespace
{
    DashboardDisplay oScreen;
    uint32_t ulLastLog = 0;
    uint16_t iLastX = 0;
    uint16_t iLastY = 0;
    bool bWasValid = false;

    void DrawReport(bool bValid, uint8_t iStatus, const uint8_t* pBytes, uint16_t iX, uint16_t iY)
    {
        oScreen.fillRect(3, 52, 314, 215, TFT_NAVY);
        oScreen.setTextSize(2);
        oScreen.setTextColor(bValid ? TFT_GREEN : TFT_YELLOW, TFT_NAVY);
        oScreen.setCursor(10, 62);
        oScreen.println(bValid ? "APPUI VALIDE" : "PAS DE POINT VALIDE");
        oScreen.setTextColor(TFT_WHITE, TFT_NAVY);
        oScreen.setCursor(10, 101);
        oScreen.printf("STATUS: 0x%02X", iStatus);
        oScreen.setCursor(10, 135);
        oScreen.printf("RAW X: %u", static_cast<unsigned>(iX));
        oScreen.setCursor(10, 165);
        oScreen.printf("RAW Y: %u", static_cast<unsigned>(iY));
        oScreen.setTextSize(1);
        oScreen.setCursor(10, 209);
        oScreen.printf("BYTES: %02X %02X %02X %02X", pBytes[0], pBytes[1], pBytes[2], pBytes[3]);
        oScreen.setCursor(10, 224);
        oScreen.printf("       %02X %02X %02X %02X", pBytes[4], pBytes[5], pBytes[6], pBytes[7]);
        oScreen.setCursor(10, 249);
        oScreen.print("X/Y little endian, debut 0x8150");
        if (bWasValid)
        {
            oScreen.fillCircle(iLastX, iLastY, 5, TFT_NAVY);
        }
        bWasValid = bValid && iX < 320 && iY < 480;
        if (bWasValid)
        {
            iLastX = iX;
            iLastY = iY;
            oScreen.fillCircle(iLastX, iLastY, 5, TFT_RED);
        }
    }
}

void setup()
{
    Serial.begin(115200);
    delay(400);
    oScreen.init();
    oScreen.setRotation(0);
    oScreen.setBrightness(220);
    oScreen.fillScreen(TFT_NAVY);
    oScreen.setTextSize(2);
    oScreen.setTextColor(TFT_CYAN, TFT_NAVY);
    oScreen.setCursor(8, 12);
    oScreen.println("GT911 RAW V0.3.15");
    oScreen.initializeTouch();
    if (!oScreen.isTouchReady())
    {
        oScreen.setTextColor(TFT_RED, TFT_NAVY);
        oScreen.setCursor(10, 60);
        oScreen.println("GT911 NON DETECTE");
        Serial.println("[GT911] I2C probe failed. See scan above.");
    }
    else
    {
        Serial.println("[GT911] Ready. Move finger; read RAW, STATUS, BYTES.");
    }
}

void loop()
{
    if (!oScreen.isTouchReady())
    {
        delay(500);
        return;
    }
    uint16_t iX = 0xFFFF;
    uint16_t iY = 0xFFFF;
    uint8_t iStatus = 0xFF;
    uint8_t aiData[8];
    memset(aiData, 0xFF, sizeof(aiData));
    const bool bValid = oScreen.getTouchRaw(&iX, &iY, &iStatus, aiData);
    if (millis() - ulLastLog >= 180)
    {
        DrawReport(bValid, iStatus, aiData, iX, iY);
        Serial.printf("[GT911] valid=%d status=0x%02X raw=(%u,%u) bytes=%02X %02X %02X %02X %02X %02X %02X %02X\n",
            bValid ? 1 : 0, iStatus, iX, iY,
            aiData[0], aiData[1], aiData[2], aiData[3],
            aiData[4], aiData[5], aiData[6], aiData[7]);
        ulLastLog = millis();
    }
    delay(15);
}
#endif
