#include "DashboardDisplay.h"
#if defined(DASH_BOARD_WAVESHARE_7)
#include <Waveshare_ST7262_LVGL.h>
#include <Arduino.h>

bool DashboardDisplay::initializeBoard()
{
    // Identical entry point to the working CantineMenuESP32 project.
    // Driver takes care of CH422G, GT911 and RGB DMA/PSRAM configuration.
    Serial.println("[LCD] Waveshare_ST7262_LVGL lcd_init...");
    lcd_init();
    Serial.println("[LCD] lcd_init complete; LVGL 8 ready");
    return true;
}
#elif defined(DASH_DISPLAY_TFT_ESPI)
#include <Arduino.h>
#include <Wire.h>
#include <cstring>

namespace
{
    constexpr uint8_t iGt911AddressA = 0x5D;
    constexpr uint8_t iGt911AddressB = 0x14;
#if defined(DASH_BOARD_CYD_28) || defined(DASH_BOARD_CYD_24)
    constexpr int iBacklightPin = 21;
#elif defined(DASH_BOARD_1732S019C)
    constexpr int iBacklightPin = 14;
#else
    constexpr int iBacklightPin = 27;
#endif
    constexpr int iBacklightChannel = 7;
#if defined(DASH_BOARD_1732S019C)
    constexpr int iTouchSda = 9;
    constexpr int iTouchScl = 46;
    constexpr int iTouchRst = 3;
#else
    constexpr int iTouchSda = 33;
    constexpr int iTouchScl = 32;
    constexpr int iTouchRst = 25;
#endif
    uint8_t iGt911Address = iGt911AddressA;
    bool ReadRegister16(uint16_t iRegister, uint8_t* pData, size_t iLength)
    {
        Wire.beginTransmission(iGt911Address);
        Wire.write(static_cast<uint8_t>(iRegister >> 8));
        Wire.write(static_cast<uint8_t>(iRegister & 0xFF));
        if (Wire.endTransmission(false) != 0)
        {
            return false;
        }
        const size_t iCount = Wire.requestFrom(static_cast<int>(iGt911Address), static_cast<int>(iLength));
        if (iCount != iLength)
        {
            return false;
        }
        for (size_t iIndex = 0; iIndex < iLength; ++iIndex)
        {
            pData[iIndex] = Wire.read();
        }
        return true;
    }
    void WriteRegister16(uint16_t iRegister, uint8_t iValue)
    {
        Wire.beginTransmission(iGt911Address);
        Wire.write(static_cast<uint8_t>(iRegister >> 8));
        Wire.write(static_cast<uint8_t>(iRegister & 0xFF));
        Wire.write(iValue);
        Wire.endTransmission();
    }
}

void DashboardDisplay::setBrightness(uint8_t iLevel)
{
    static bool bInitialized = false;
    if (!bInitialized)
    {
        ledcSetup(iBacklightChannel, 12000, 8);
        ledcAttachPin(iBacklightPin, iBacklightChannel);
        bInitialized = true;
    }
    ledcWrite(iBacklightChannel, iLevel);
}

#if defined(DASH_BOARD_CYD_28) || defined(DASH_BOARD_CYD_24)
void DashboardDisplay::initializeTouch()
{
    // CYD uses its OWN touch SPI bus: SCLK=25, MISO=39, MOSI=32, CS=33.
    // Do not configure Wire/GT911 (used by the 035C only).
    oTouchBus.begin(25, 39, 32, 33);
    bTouchReady = oTouch.begin(oTouchBus);
    oTouch.setRotation(0);
    Serial.printf("[TOUCH] CYD XPT2046 on HSPI, ready=%d (calibration provisional)\n", bTouchReady);
}

bool DashboardDisplay::isTouchReady() const
{
    return bTouchReady;
}

bool DashboardDisplay::getTouchRaw(uint16_t* pX, uint16_t* pY, uint8_t* pStatus, uint8_t* pData)
{
    if (pX == nullptr || pY == nullptr || !bTouchReady || !oTouch.touched()) return false;
    TS_Point tPoint = oTouch.getPoint();
    *pX = static_cast<uint16_t>(constrain(tPoint.x, 0, 4095));
    *pY = static_cast<uint16_t>(constrain(tPoint.y, 0, 4095));
    if (pStatus != nullptr) *pStatus = 0x81;
    if (pData != nullptr) memset(pData, 0, 8);
    return true;
}

bool DashboardDisplay::getTouch(uint16_t* pX, uint16_t* pY)
{
    uint16_t iRawX = 0, iRawY = 0;
    if (!getTouchRaw(&iRawX, &iRawY, nullptr, nullptr)) return false;
    // A starting calibration only. The physical touch orientation must be tested
    // on each 028/024 revision before use with actionable Jeedom commands.
    int iX = map(iRawX, 200, 3900, 0, 239);
    int iY = map(iRawY, 200, 3900, 0, 319);
#if DASH_ROTATION == 1
    const int iOriginalX = iX;
    iX = iY;
    iY = 239 - iOriginalX;
#endif
    *pX = static_cast<uint16_t>(constrain(iX, 0, static_cast<int>(dash::iWidth) - 1));
    *pY = static_cast<uint16_t>(constrain(iY, 0, static_cast<int>(dash::iHeight) - 1));
    return true;
}
#else
void DashboardDisplay::initializeTouch()
{
    bTouchReady = false;
    Wire.begin(iTouchSda, iTouchScl, 100000);
    Wire.setTimeOut(50);

    // GT911 pins are selected for 035C or 1732S019C above.
    // IRQ can be connected to GND on this board: DO NOT depend on interrupts.
    // Pulse RESET before probing; some GT911 revisions do not respond
    // following a microcontroller-only reset without a reset pulse.
    pinMode(iTouchRst, OUTPUT);
    digitalWrite(iTouchRst, LOW);
    delay(30);
    digitalWrite(iTouchRst, HIGH);
    delay(180);

    for (int iAttempt = 0; iAttempt < 3 && !bTouchReady; ++iAttempt)
    {
        for (uint8_t iAddress : {iGt911AddressA, iGt911AddressB})
        {
            iGt911Address = iAddress;
            Wire.beginTransmission(iAddress);
            if (Wire.endTransmission() != 0)
            {
                continue;
            }
            uint8_t aiProductId[4] = {0};
            if (!ReadRegister16(0x8140, aiProductId, sizeof(aiProductId)))
            {
                Serial.printf("[TOUCH] ACK 0x%02X but GT911 product-id read failed\n", iAddress);
                continue;
            }
            bTouchReady = true;
            Serial.printf("[TOUCH] GT911 found at 0x%02X; product ID: %c%c%c%c\n",
                          iGt911Address, aiProductId[0], aiProductId[1],
                          aiProductId[2], aiProductId[3]);
            break;
        }
        if (!bTouchReady)
        {
            Serial.printf("[TOUCH] Attempt %d/3 failed; retrying I2C probe\n", iAttempt+1);
            delay(150);
        }
    }
    if (!bTouchReady)
    {
        Serial.printf("[TOUCH] GT911 NOT DETECTED on SDA=%d SCL=%d (0x5D / 0x14)\n", iTouchSda, iTouchScl);
        Serial.println("[TOUCH] I2C scan:");
        for (uint8_t iAddress = 8; iAddress < 120; ++iAddress)
        {
            Wire.beginTransmission(iAddress);
            if (Wire.endTransmission() == 0)
            {
                Serial.printf("[TOUCH] I2C device: 0x%02X\n", iAddress);
            }
        }
    }
}

bool DashboardDisplay::isTouchReady() const
{
    return bTouchReady;
}

bool DashboardDisplay::getTouchRaw(uint16_t* pX, uint16_t* pY, uint8_t* pStatus, uint8_t* pData)
{
    if (pX == nullptr || pY == nullptr || !bTouchReady)
    {
        return false;
    }
    *pX = 0xFFFF;
    *pY = 0xFFFF;
    if (pStatus != nullptr) *pStatus = 0xFF;
    if (pData != nullptr) memset(pData, 0xFF, 8);

    uint8_t iStatus = 0;
    if (!ReadRegister16(0x814E, &iStatus, 1))
    {
        return false;
    }
    if (pStatus != nullptr) *pStatus = iStatus;
    if ((iStatus & 0x80) == 0)
    {
        return false;
    }
    const uint8_t iCount = iStatus & 0x0F;
    if (iCount == 0 || iCount > 5)
    {
        WriteRegister16(0x814E, 0);
        return false;
    }
    uint8_t aiData[8] = {0};
    const bool bRead = ReadRegister16(0x8150, aiData, sizeof(aiData));
    WriteRegister16(0x814E, 0);
    if (!bRead)
    {
        return false;
    }
    if (pData != nullptr) memcpy(pData, aiData, sizeof(aiData));

    // GT911 point block at 0x8150 starts with X_LOW, X_HIGH,
    // Y_LOW, Y_HIGH. The TRACK_ID is at 0x814F, one register BEFORE
    // this read, and is intentionally not included in aiData.
    // Old code incorrectly skipped aiData[0], producing out-of-range
    // coordinates such as (21504,11521) for a real touch (174,340).
    const uint16_t iRawX = static_cast<uint16_t>(aiData[0] | (aiData[1] << 8));
    const uint16_t iRawY = static_cast<uint16_t>(aiData[2] | (aiData[3] << 8));
    *pX = iRawX;
    *pY = iRawY;
    // 0xFFFF is a common signature of invalid data; never clamp it to
    // the bottom-right corner (319,479), which hides the real problem.
    return iRawX != 0xFFFF && iRawY != 0xFFFF && iRawX < 4096 && iRawY < 4096;
}

bool DashboardDisplay::getTouch(uint16_t* pX, uint16_t* pY)
{
    if (pX == nullptr || pY == nullptr)
    {
        return false;
    }
    uint16_t iRawX = 0;
    uint16_t iRawY = 0;
    if (!getTouchRaw(&iRawX, &iRawY, nullptr, nullptr))
    {
        return false;
    }
#if defined(DASH_BOARD_1732S019C)
    // C variant: 170x320 native. Validate raw-axis orientation on real hardware.
    const int iX = DASH_ROTATION ? static_cast<int>(iRawY) : static_cast<int>(iRawX);
    const int iY = DASH_ROTATION ? 169 - static_cast<int>(iRawX) : static_cast<int>(iRawY);
    if (iX < 0 || iX >= dash::iWidth || iY < 0 || iY >= dash::iHeight) return false;
#elif DASH_ROTATION == 1
    // Mapping is preliminary until raw GT911 axes are validated.
    const int iX = static_cast<int>(iRawY);
    const int iY = 319 - static_cast<int>(iRawX);
    if (iX < 0 || iX >= 480 || iY < 0 || iY >= 320) return false;
#else
    const int iX = static_cast<int>(iRawX);
    const int iY = static_cast<int>(iRawY);
    if (iX < 0 || iX >= 320 || iY < 0 || iY >= 480) return false;
#endif
    *pX = static_cast<uint16_t>(iX);
    *pY = static_cast<uint16_t>(iY);
    return true;
}
#endif // CYD XPT2046 vs GT911 035C

#else

// I2C_NUM_1 is defined by the ESP-IDF I2C driver, not by LovyanGFX.
#if defined(DASH_TOUCH_CAPACITIVE)
#include <driver/i2c.h>
#endif

DashboardDisplay::DashboardDisplay()
{
    {
        auto tConfig = oBus.config();
        tConfig.spi_host = VSPI_HOST;
        tConfig.spi_mode = 0;
        tConfig.freq_write = dash::ulLcdSpiHz;
        tConfig.freq_read = dash::ulLcdSpiReadHz;
        tConfig.spi_3wire = false;
        tConfig.use_lock = true;
        tConfig.dma_channel = 1;
        tConfig.pin_sclk = dash::iLcdSclk;
        tConfig.pin_mosi = dash::iLcdMosi;
        tConfig.pin_miso = dash::iLcdMiso;
        tConfig.pin_dc = dash::iLcdDc;
        oBus.config(tConfig);
        oPanel.setBus(&oBus);
    }

    {
        auto tConfig = oPanel.config();
        tConfig.pin_cs = dash::iLcdCs;
        tConfig.pin_rst = dash::iLcdRst;
        tConfig.pin_busy = -1;
        tConfig.panel_width = dash::iNativeWidth;
        tConfig.panel_height = dash::iNativeHeight;
        tConfig.memory_width = dash::iNativeWidth;
        tConfig.memory_height = dash::iNativeHeight;
        tConfig.offset_x = 0;
        tConfig.offset_y = 0;
        tConfig.offset_rotation = 0;
        tConfig.dummy_read_pixel = 8;
        tConfig.dummy_read_bits = 1;
        tConfig.readable = true;
        tConfig.invert = true;
        tConfig.rgb_order = false;
        tConfig.dlen_16bit = false;
        tConfig.bus_shared = true;
        oPanel.config(tConfig);
    }

    {
        auto tConfig = oLight.config();
        tConfig.pin_bl = dash::iLcdBl;
        tConfig.invert = false;
        tConfig.freq = 12000;
        tConfig.pwm_channel = 7;
        oLight.config(tConfig);
        oPanel.setLight(&oLight);
    }

    {
        auto tConfig = oTouch.config();
        tConfig.x_min = 0;
        tConfig.x_max = dash::iNativeWidth - 1;
        tConfig.y_min = 0;
        tConfig.y_max = dash::iNativeHeight - 1;

#if defined(DASH_TOUCH_CAPACITIVE)
        tConfig.pin_int = dash::iTouchInt;
        tConfig.pin_rst = dash::iTouchRst;
        tConfig.bus_shared = false;
        tConfig.offset_rotation = 0;
        tConfig.i2c_port = I2C_NUM_1;
        tConfig.i2c_addr = 0x5D; // Some GT911 revisions use address 0x14.
        tConfig.pin_sda = dash::iTouchSda;
        tConfig.pin_scl = dash::iTouchScl;
        tConfig.freq = 400000;
#else
        // Initial raw values must be calibrated against the physical board.
        tConfig.x_min = 200;
        tConfig.x_max = 3900;
        tConfig.y_min = 200;
        tConfig.y_max = 3900;
        tConfig.pin_int = -1;
        tConfig.bus_shared = true;
        tConfig.offset_rotation = 0;
        tConfig.spi_host = VSPI_HOST;
        tConfig.freq = 2500000;
        tConfig.pin_sclk = dash::iLcdSclk;
        tConfig.pin_mosi = dash::iLcdMosi;
        tConfig.pin_miso = dash::iLcdMiso;
        tConfig.pin_cs = dash::iTouchCs;
#endif

        oTouch.config(tConfig);
        oPanel.setTouch(&oTouch);
    }

    setPanel(&oPanel);
}


#endif
