#if defined(DASH_BOARD_WAVESHARE_7)
#include <Arduino.h>
#include <lvgl.h>
#include <Waveshare_ST7262_LVGL.h>
#include "DashboardConfig.h"
#include "DashboardDisplay.h"
#if defined(DASH_OFFLINE_DEMO)
#include "DashboardDemo.h"
#elif !defined(DASH_LCD_DIAGNOSTIC) && !defined(DASH_TOUCH_DIAGNOSTIC)
#include "DashboardRuntime.h"
#endif

namespace
{
    DashboardDisplay oDisplay;
#if !defined(DASH_LCD_DIAGNOSTIC) && !defined(DASH_TOUCH_DIAGNOSTIC) && !defined(DASH_OFFLINE_DEMO)
    DashboardRuntime oRuntime;
#endif
#if defined(DASH_OFFLINE_DEMO)
    DashboardDemo oDemo;
#endif

#if defined(DASH_TOUCH_DIAGNOSTIC)
    lv_obj_t* pTouchCoordinates = nullptr;
    lv_obj_t* pTouchStatus = nullptr;
    lv_obj_t* pTouchMarker = nullptr;
    uint32_t ulTouchCount = 0;
    uint32_t ulLastTouchLog = 0;
    bool bTouchActive = false;

    void UpdateTouchPosition(int32_t iX, int32_t iY, bool bPressed)
    {
        if (pTouchCoordinates == nullptr || pTouchStatus == nullptr || pTouchMarker == nullptr)
        {
            return;
        }

        if (bPressed)
        {
            char acCoordinates[64];
            snprintf(acCoordinates, sizeof(acCoordinates), "X: %ld    Y: %ld", static_cast<long>(iX), static_cast<long>(iY));
            lv_label_set_text(pTouchCoordinates, acCoordinates);
            lv_obj_set_pos(pTouchMarker, static_cast<lv_coord_t>(iX - 13), static_cast<lv_coord_t>(iY - 13));
            lv_obj_clear_flag(pTouchMarker, LV_OBJ_FLAG_HIDDEN);

            if (!bTouchActive)
            {
                ++ulTouchCount;
                char acStatus[80];
                snprintf(acStatus, sizeof(acStatus), "APPUI #%lu  -  EN COURS", static_cast<unsigned long>(ulTouchCount));
                lv_label_set_text(pTouchStatus, acStatus);
                Serial.printf("[TOUCH] DOWN #%lu  X=%ld  Y=%ld\n", static_cast<unsigned long>(ulTouchCount), static_cast<long>(iX), static_cast<long>(iY));
                bTouchActive = true;
            }
            else if (millis() - ulLastTouchLog >= 150)
            {
                Serial.printf("[TOUCH] MOVE      X=%ld  Y=%ld\n", static_cast<long>(iX), static_cast<long>(iY));
            }
            ulLastTouchLog = millis();
        }
        else if (bTouchActive)
        {
            lv_label_set_text(pTouchStatus, "RELACHE - toucher de nouveau");
            lv_obj_add_flag(pTouchMarker, LV_OBJ_FLAG_HIDDEN);
            Serial.printf("[TOUCH] UP        X=%ld  Y=%ld\n", static_cast<long>(iX), static_cast<long>(iY));
            bTouchActive = false;
        }
    }

    void TouchScreenEvent(lv_event_t* pEvent)
    {
        const lv_event_code_t eCode = lv_event_get_code(pEvent);
        if (eCode != LV_EVENT_PRESSED && eCode != LV_EVENT_PRESSING && eCode != LV_EVENT_RELEASED && eCode != LV_EVENT_PRESS_LOST)
        {
            return;
        }
        lv_indev_t* pInput = lv_indev_get_act();
        lv_point_t tPoint = {0, 0};
        if (pInput != nullptr)
        {
            lv_indev_get_point(pInput, &tPoint);
        }
        UpdateTouchPosition(tPoint.x, tPoint.y, eCode == LV_EVENT_PRESSED || eCode == LV_EVENT_PRESSING);
    }

    lv_obj_t* CreateDisplayLabel(lv_obj_t* pParent, const char* sText, int32_t iX, int32_t iY, uint32_t ulColor)
    {
        lv_obj_t* pLabel = lv_label_create(pParent);
        lv_label_set_text(pLabel, sText);
        lv_obj_set_style_text_color(pLabel, lv_color_hex(ulColor), 0);
        lv_obj_set_pos(pLabel, static_cast<lv_coord_t>(iX), static_cast<lv_coord_t>(iY));
        lv_obj_clear_flag(pLabel, LV_OBJ_FLAG_CLICKABLE);
        return pLabel;
    }

    void BuildTouchDiagnostic()
    {
        lv_obj_t* pScreen = lv_scr_act();
        lv_obj_set_style_bg_color(pScreen, lv_color_hex(0x101827), 0);
        lv_obj_set_style_bg_opa(pScreen, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_all(pScreen, 0, 0);
        lv_obj_clear_flag(pScreen, LV_OBJ_FLAG_SCROLLABLE);

        // Dedicated transparent hit area: full panel and above the background.
        lv_obj_t* pHitArea = lv_obj_create(pScreen);
        lv_obj_set_pos(pHitArea, 0, 0);
        lv_obj_set_size(pHitArea, 800, 480);
        lv_obj_set_style_bg_opa(pHitArea, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(pHitArea, 0, 0);
        lv_obj_set_style_radius(pHitArea, 0, 0);
        lv_obj_set_style_pad_all(pHitArea, 0, 0);
        lv_obj_clear_flag(pHitArea, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(pHitArea, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(pHitArea, TouchScreenEvent, LV_EVENT_ALL, nullptr);

        CreateDisplayLabel(pHitArea, "DASHBOARDESP V0.3.0 - TEST TACTILE GT911", 20, 18, 0xFFFFFF);
        CreateDisplayLabel(pHitArea, "Toucher les 4 coins, le centre et glisser le doigt", 20, 52, 0xB3C6E8);
        CreateDisplayLabel(pHitArea, "(0,0)", 6, 94, 0x00FFAA);
        CreateDisplayLabel(pHitArea, "(799,0)", 720, 94, 0x00FFAA);
        CreateDisplayLabel(pHitArea, "(0,479)", 6, 448, 0x00FFAA);
        CreateDisplayLabel(pHitArea, "(799,479)", 697, 448, 0x00FFAA);
        pTouchCoordinates = CreateDisplayLabel(pHitArea, "X: ---    Y: ---", 284, 205, 0xFFFF00);
        pTouchStatus = CreateDisplayLabel(pHitArea, "EN ATTENTE D'UN APPUI", 284, 240, 0xFFFFFF);

        pTouchMarker = lv_obj_create(pHitArea);
        lv_obj_set_size(pTouchMarker, 26, 26);
        lv_obj_set_style_radius(pTouchMarker, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(pTouchMarker, lv_color_hex(0xFF4433), 0);
        lv_obj_set_style_bg_opa(pTouchMarker, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(pTouchMarker, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(pTouchMarker, 2, 0);
        lv_obj_clear_flag(pTouchMarker, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(pTouchMarker, LV_OBJ_FLAG_HIDDEN);
        Serial.println("[TOUCH] GT911 test ready. Touch the corners and slide across the screen.");
    }
#endif
}

void setup()
{
    Serial.begin(115200);
    delay(600);
    Serial.printf("[DASH] Firmware V%s / Waveshare 7 LCD / LVGL %d.%d.%d\n",
                  DASH_VERSION, LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
    Serial.printf("[MEMORY] PSRAM available=%lu, free=%lu\n",
                  static_cast<unsigned long>(ESP.getPsramSize()),
                  static_cast<unsigned long>(ESP.getFreePsram()));
    if (!psramFound())
    {
        Serial.println("[ERROR] No PSRAM; check flash/OPI settings");
        while (true) { delay(1000); }
    }

    if (!oDisplay.initializeBoard())
    {
        Serial.println("[ERROR] Waveshare LCD initialization failed");
        while (true) { delay(1000); }
    }

#if defined(DASH_TOUCH_DIAGNOSTIC)
    Serial.println("[TEST] GT911 touch diagnostic, no WiFi and no JSON");
    if (lvgl_port_lock(-1))
    {
        BuildTouchDiagnostic();
        lvgl_port_unlock();
    }
    else
    {
        Serial.println("[ERROR] LVGL lock failed during touch test startup");
    }
#elif defined(DASH_OFFLINE_DEMO)
    Serial.println("[DEMO] DashboardESP offline widgets and navigation");
    if (lvgl_port_lock(-1))
    {
        oDemo.begin();
        lvgl_port_unlock();
    }
#elif defined(DASH_LCD_DIAGNOSTIC)
    Serial.println("[TEST] RGB screen diagnostic, no WiFi and no JSON");
    if (lvgl_port_lock(-1))
    {
        lv_obj_t* pScreen = lv_scr_act();
        lv_obj_set_style_bg_color(pScreen, lv_color_hex(0x101827), 0);
        lv_obj_set_style_pad_all(pScreen, 0, 0);
        constexpr uint32_t aiColors[6] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFFFF, 0x000000, 0xFFFF00};
        for (int iIndex = 0; iIndex < 6; ++iIndex)
        {
            lv_obj_t* pBar = lv_obj_create(pScreen);
            lv_obj_set_pos(pBar, 0, iIndex * 80);
            lv_obj_set_size(pBar, 800, 80);
            lv_obj_set_style_bg_color(pBar, lv_color_hex(aiColors[iIndex]), 0);
            lv_obj_set_style_bg_opa(pBar, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(pBar, 0, 0);
            lv_obj_set_style_radius(pBar, 0, 0);
            lv_obj_set_style_pad_all(pBar, 0, 0);
        }
        lv_obj_t* pTitle = lv_label_create(pScreen);
        lv_label_set_text(pTitle, "DASHBOARD ESP - LCD TEST - WAVESHARE DRIVER");
        lv_obj_set_style_text_color(pTitle, lv_color_hex(0x000000), 0);
        lv_obj_align(pTitle, LV_ALIGN_CENTER, 0, -5);
        lvgl_port_unlock();
    }
#else
    Serial.println("[READY] Starting DashboardESP runtime");
    if (lvgl_port_lock(-1))
    {
        oRuntime.begin();
        lvgl_port_unlock();
    }
#endif
}

void loop()
{
#if defined(DASH_LCD_DIAGNOSTIC) || defined(DASH_TOUCH_DIAGNOSTIC)
    delay(1000);
#else
    // The Waveshare library already services LVGL in its own task.
    // Never call lv_init(), lv_timer_handler() or lv_tick_inc() again here.
    if (lvgl_port_lock(100))
    {
#if defined(DASH_OFFLINE_DEMO)
        oDemo.loop();
#else
        oRuntime.loop();
#endif
        lvgl_port_unlock();
    }
    delay(5);
#endif
}

#else
#include <Arduino.h>
#include <lvgl.h>

#include "DashboardConfig.h"
#include "DashboardDisplay.h"
#include "DashboardRuntime.h"

namespace
{
    DashboardDisplay oLcd;
    DashboardRuntime oRuntime;
    lv_display_t* pLvDisplay = nullptr;
    lv_indev_t* pTouchInput = nullptr;

    // 7-inch RGB requires PSRAM for framebuffer (handled by LovyanGFX).
    // LVGL partial draw buffer stays in internal RAM.
    static uint16_t aiDrawBuffer[dash::iNativeWidth * dash::iDrawBufferLines];
    uint32_t ulLastTick = 0;
    uint32_t ulLastTouchLog = 0;

    void FlushCallback(lv_display_t* pDisplay, const lv_area_t* pArea, uint8_t* pPixelMap)
    {
        const int32_t iWidth = pArea->x2 - pArea->x1 + 1;
        const int32_t iHeight = pArea->y2 - pArea->y1 + 1;
#if defined(DASH_DISPLAY_TFT_ESPI)
        // TFT_eSPI is the exact ST7796 transport used in the working project.
        // LVGL RGB565 pixels are byte-swapped for TFT_eSPI pushImage().
        oLcd.startWrite();
        oLcd.pushImage(pArea->x1, pArea->y1, iWidth, iHeight,
                       reinterpret_cast<uint16_t*>(pPixelMap));
        oLcd.endWrite();
#else
        oLcd.startWrite();
        oLcd.setAddrWindow(pArea->x1, pArea->y1, iWidth, iHeight);
        oLcd.writePixels(reinterpret_cast<lgfx::rgb565_t*>(pPixelMap),
                         static_cast<uint32_t>(iWidth * iHeight), true);
        oLcd.endWrite();
#endif
        lv_display_flush_ready(pDisplay);
    }

    void TouchCallback(lv_indev_t* pDevice, lv_indev_data_t* pData)
    {
        (void)pDevice;
        uint16_t iX = 0;
        uint16_t iY = 0;
        const bool bTouched = oLcd.getTouch(&iX, &iY);

        if (!bTouched)
        {
            pData->state = LV_INDEV_STATE_RELEASED;
            oRuntime.showTouch(0, 0, false);
            return;
        }

        pData->state = LV_INDEV_STATE_PRESSED;
        pData->point.x = iX;
        pData->point.y = iY;
        oRuntime.showTouch(iX, iY, true);

        if (millis() - ulLastTouchLog > 500)
        {
            ulLastTouchLog = millis();
            Serial.printf("[TOUCH] x=%u y=%u\n", iX, iY);
        }
    }
}

void setup()
{
    Serial.begin(115200);
    delay(200);
    Serial.printf("\nDashboardESP V%s | %s | %s | %u x %u\n",
                  DASH_VERSION, dash::sBoardName, dash::sOrientationName,
                  dash::iWidth, dash::iHeight);

#if defined(DASH_BOARD_WAVESHARE_7)
    Serial.printf("[MEMORY] PSRAM: %u octets, free: %u octets\n",
                  ESP.getPsramSize(), ESP.getFreePsram());

    if (!psramFound())
    {
        Serial.println("[ERROR] PSRAM absente. Verifier le profil OPI PSRAM.");
        while (true)
        {
            delay(1000);
        }
    }

    if (!oLcd.initializeBoard())
    {
        Serial.println("[ERROR] Initialisation LCD/tactile impossible.");
        while (true)
        {
            delay(1000);
        }
    }
#else
    oLcd.init();
#if defined(DASH_DISPLAY_TFT_ESPI)
    oLcd.setSwapBytes(true);
    oLcd.initializeTouch();
#endif
#endif
    oLcd.setRotation(DASH_ROTATION);
#if !defined(DASH_BOARD_WAVESHARE_7)
    oLcd.setBrightness(220);
#endif
    oLcd.fillScreen(TFT_BLACK);

#if defined(DASH_BOARD_WAVESHARE_7) && defined(DASH_LCD_DIAGNOSTIC)
    // Hardware-only diagnostics: bypass LVGL, Wi-Fi and JSON rendering.
    // Six bars must be stable, horizontal and cover the entire 800x480 panel.
    Serial.println("[DIAG] RGB565 direct framebuffer test (LVGL disabled)");
    const uint32_t aColors[] = {
        TFT_RED, TFT_GREEN, TFT_BLUE,
        TFT_WHITE, TFT_BLACK, TFT_YELLOW
    };

    for (uint8_t iIndex = 0; iIndex < 6; iIndex++)
    {
        oLcd.fillRect(0, iIndex * 80, 800, 80, aColors[iIndex]);
    }

    oLcd.setTextSize(2);
    oLcd.setTextColor(TFT_BLACK, TFT_WHITE);
    oLcd.setCursor(15, 244);
    oLcd.print("DashboardESP RGB TEST - 800x480");
    Serial.println("[DIAG] Colors drawn; picture should remain stable");
    return;
#endif

    lv_init();
    pLvDisplay = lv_display_create(oLcd.width(), oLcd.height());
    lv_display_set_color_format(pLvDisplay, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(pLvDisplay, aiDrawBuffer, nullptr,
                           sizeof(aiDrawBuffer), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(pLvDisplay, FlushCallback);
    pTouchInput = lv_indev_create();
    lv_indev_set_type(pTouchInput, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(pTouchInput, TouchCallback);
    lv_indev_set_display(pTouchInput, pLvDisplay);
    oRuntime.begin();
    ulLastTick = millis();
    Serial.println("[READY] API, Wi-Fi, LVGL started");
}

void loop()
{
#if defined(DASH_BOARD_WAVESHARE_7) && defined(DASH_LCD_DIAGNOSTIC)
    delay(1000);
    return;
#endif
    const uint32_t ulNow = millis();
    lv_tick_inc(ulNow - ulLastTick);
    ulLastTick = ulNow;
    lv_timer_handler();
    oRuntime.loop();
    delay(5);
}

#endif
