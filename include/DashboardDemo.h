#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <lvgl.h>

// Offline functional test of LVGL widgets, navigation and GT911 events.
// Does not modify the proven Waveshare LCD/GT911 driver.
class DashboardDemo
{
public:
    DashboardDemo();
    void begin();
    void loop();

private:
    struct DemoWidget
    {
        DashboardDemo* pOwner = nullptr;
        String sType;
        String sTarget;
        int iCommandId = 0;
        lv_obj_t* pButton = nullptr;
    };

    DynamicJsonDocument oProject;
    DemoWidget atButtons[16];
    int iButtonCount = 0;
    int iActionCount = 0;
    bool bLightOn = false;
    String sPendingPage;

    void renderPage(const String& sPageId);
    void renderWidget(JsonObjectConst tWidget, lv_obj_t* pScreen);
    static void onWidgetClick(lv_event_t* pEvent);
    static uint32_t parseColor(const String& sValue, uint32_t ulFallback);
};
