#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <FS.h>
#include "DashboardLVGLCompat.h"

class DashboardRuntime
{
public:
    DashboardRuntime();
    void begin();
    void loop();
    void showTouch(uint16_t iX, uint16_t iY, bool bPressed);

private:
    // Waveshare S3 boards with OPI PSRAM may handle a 48 KiB config.
    // Reserve a larger ArduinoJson pool for parsed nodes; keep classic ESP32
    // strictly at 24 KiB (or less, if the fallback pool is smaller).
#if defined(DASH_BOARD_WAVESHARE_7)
    static constexpr size_t iJsonCapacity = 65536;
    static constexpr size_t iMaxConfigBytes = 49152;
#else
    static constexpr size_t iJsonCapacity = 24576;
    static constexpr size_t iMaxConfigBytes = 24576;
#endif
    static constexpr int iMaxWidgetsPerPage = 40;
    static constexpr int iMaxPages = 12;
    static constexpr int iMaxValues = 96;
    static constexpr int iMaxActions = 12;

    struct ValueEntry
    {
        int iCommandId = 0;
        String sValue;
        bool bValid = false;
    };

    struct WidgetEntry
    {
        DashboardRuntime* pOwner = nullptr;
        lv_obj_t* pObject = nullptr;
        lv_obj_t* pLabel = nullptr;
        lv_obj_t* pAux = nullptr;
        lv_obj_t* pBar = nullptr;
        lv_obj_t* pMinus = nullptr;
        lv_obj_t* pPlus = nullptr;
        lv_obj_t* pModeComfort = nullptr;
        lv_obj_t* pModeEco = nullptr;
        lv_obj_t* pModeOff = nullptr;
        lv_obj_t* apMetricLabels[4] = {};
        lv_obj_t* pWeatherIcon = nullptr;
        lv_obj_t* apChoiceButtons[8] = {};
        lv_obj_t* apChoiceLabels[8] = {};
        int aiChoiceActions[8] = {};
        String asChoiceValues[8];
        int iChoiceCount = 0;
        bool abChoiceActive[8] = {};
        lv_obj_t* pWeatherInfo = nullptr;
        String sId;
        String sType;
        String sTargetPage;
        int iActionId = 0;
        int iOnActionId = 0;
        int iOffActionId = 0;
        lv_obj_t* pBulbHead = nullptr;
        lv_obj_t* pBulbStem = nullptr;
        int iInfoId = 0;
        float fThermostatSetpoint = NAN;
        bool bThermostatPending = false;
        JsonObjectConst tSource;
    };

    WebServer oServer;
    WebServer oPortal;
    DNSServer oDns;
    Preferences oPreferences;
    // Keep mutable JSON bytes alive: ArduinoJson uses zero-copy when given char*.
    // WidgetEntry::tSource refers to strings stored inside this buffer.
    String sProjectJsonBacking;
    DynamicJsonDocument oProject;
    ValueEntry atValues[iMaxValues];
    WidgetEntry atWidgets[iMaxWidgetsPerPage];
    int iWidgetCount = 0;
    int iValueCount = 0;
    int iActionCount = 0;
    struct ActionEntry
    {
        int iCommandId = 0;
        bool bHasValue = false;
        float fValue = 0;
        bool bHasChoice = false;
        String sChoice;
    };
    ActionEntry atActions[iMaxActions];
    String sCurrentPage;
    String sPendingPage;
    String sDeviceName;
    String sWifiSsid;
    String sWifiPassword;
    String sAdminToken;
    String sJeedomActionUrl;
    String sJeedomActionKey;
    int iJeedomDeviceId = 0;
    bool bDirectActionConfigured = false;
    uint32_t ulActionSequence = 0;
    void configureActionTransport();
    void processDirectActions();
    String sLastError;
    bool bAccessPoint = false;
    bool bSetupMode = false;
    bool bProjectReady = false;
    bool bPortalRequested = false;
    bool bInfoVisible = false;
    bool bInfoReturnSetup = false;
    bool bInfoTokenRevealed = false;
    uint32_t ulLastUserActivity = 0;
    uint32_t ulLastClockPosition = 0;
    bool bSaverActive = false;
    bool bSaverBlack = false;
    bool bSaverWakeRequested = false;
    uint16_t iSaverClockMinute = 5;
    uint16_t iSaverBlackAdditionalMinute = 15;
    uint16_t iSaverMoveSeconds = 10;
    lv_obj_t* pSaverOverlay = nullptr;
    lv_obj_t* pSaverClock = nullptr;
    void updateSaver();
    void wakeSaver();
    static void saverTouchEvent(lv_event_t* pEvent);
    uint32_t ulLastWifiAttempt = 0;
    uint32_t ulTouchDown = 0;
    bool bTouchHoldTriggered = false;
    uint32_t ulLastScreenUpdate = 0;
    uint32_t ulLastDiagnostic = 0;
    float fCanvasScale = 1.0f;
    int iCanvasOffsetX = 0;
    int iCanvasOffsetY = 0;
    // One quarter-resolution RGB565 background at a time (<= 80 KiB).
    uint8_t* pBackgroundPixels = nullptr;
    lv_obj_t* pBackgroundImage = nullptr;
#if defined(DASH_LVGL8)
    lv_img_dsc_t tBackgroundDescriptor = {};
#else
    lv_image_dsc_t tBackgroundDescriptor = {};
#endif
    void drawPageBackground(JsonObjectConst tPage);
    File oBackgroundUploadFile;
    bool bBackgroundUploadOk = false;
    size_t iBackgroundUploadBytes = 0;
    String sBackgroundUploadTemp;
    String sBackgroundUploadFinal;
    lv_obj_t* pScreen = nullptr;
    lv_obj_t* pStatus = nullptr;
    lv_obj_t* pWifiSsid = nullptr;
    lv_obj_t* pWifiPassword = nullptr;
    lv_obj_t* pDeviceName = nullptr;
    lv_obj_t* pKeyboard = nullptr;

    void loadPreferences();
    void startWifi();
    void startAccessPoint();
    void registerRoutes();
    bool authorize();
    void sendJson(int iStatus, const String& sJson);
    void showSetup();
    void showDefault();
    void showInfo();
    void addInfoButton(lv_obj_t* pParent);
    static void infoOpenEvent(lv_event_t* pEvent);
    static void infoRevealEvent(lv_event_t* pEvent);
    static void infoBackEvent(lv_event_t* pEvent);
    static void infoWifiEvent(lv_event_t* pEvent);
    void renderPage(const String& sPageId);
    void renderWidget(JsonObjectConst tWidget);
    void styleWidget(WidgetEntry& tEntry);
    void updateWidgets();
    void updateWidget(WidgetEntry& tEntry);
    void setValue(int iCommandId, const String& sValue);
    String getValue(int iCommandId, const String& sFallback) const;
    String getBinding(JsonObjectConst tBindings, const char* sKey, const String& sFallback) const;
    static int readCommand(JsonVariantConst tValue);
    static uint32_t parseColor(const String& sColor, uint32_t ulFallback);
    static bool numericValue(const String& sValue, float& fValue);
    void resolveThreshold(JsonObjectConst tWidget, const String& sValue, uint32_t& ulText, uint32_t& ulBackground) const;
    static const lv_font_t* selectFont(int iSize);
    int scaledFont(JsonObjectConst tWidget, int iDefault) const;
    void handleSerial();
    void processSerialCommand(String sCommand);
    static void widgetEvent(lv_event_t* pEvent);
    static void thermostatEvent(lv_event_t* pEvent);
    static void thermostatModeEvent(lv_event_t* pEvent);
    void queueAction(int iCommandId, bool bHasValue = false, float fValue = 0);
    void queueChoiceAction(int iCommandId, const String& sChoice);
    static void choiceEvent(lv_event_t* pEvent);
    static void setupEvent(lv_event_t* pEvent);
    static void textareaEvent(lv_event_t* pEvent);
    static void rootLongPress(lv_event_t* pEvent);
    bool loadProject();
    bool saveProject(const String& sJson);
    bool validateProject(JsonVariantConst tProject, String& sError);
    void resetWidgets();
    void refreshStatus();
};
