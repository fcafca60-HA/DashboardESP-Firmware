#include "DashboardRuntime.h"
#include "DashboardConfig.h"

// Network.h exists with Arduino-ESP32 3.x (Waveshare), not 2.x (ESP32-3248S035).
// The 3.5-inch profiles use the WiFi 2.x API directly.
#if defined(DASH_BOARD_WAVESHARE_7)
#include <Network.h>
#endif
#include <WiFi.h>
#include <LittleFS.h>
#include <HTTPClient.h>
#include <esp_system.h>
#include <time.h>
#include <math.h>

namespace
{
    constexpr uint32_t ulBackground = 0x101827;
    constexpr uint32_t ulForeground = 0xFFFFFF;
    constexpr uint32_t ulAccent = 0x316AC4;
    constexpr uint16_t iApiPort = 8090;
    constexpr uint32_t ulWifiRetryMs = 15000;
    constexpr uint32_t ulRefreshMs = 1000;
    const char* sProjectFile = "/project.json";
    const char* sProjectBackup = "/project.bak";
    const char* sProjectTemporary = "/project.tmp";

    void SetText(lv_obj_t* pLabel, const String& sText)
    {
        if (pLabel != nullptr)
        {
            lv_label_set_text(pLabel, sText.c_str());
        }
    }

    String HtmlEscape(const String& sValue)
    {
        String sResult = sValue;
        sResult.replace("&", "&amp;");
        sResult.replace("<", "&lt;");
        sResult.replace(">", "&gt;");
        sResult.replace("\"", "&quot;");
        return sResult;
    }

    const char* SymbolText(const String& sKey)
    {
        if (sKey == "light")
        {
            return "L";
        }
        if (sKey == "heating")
        {
            return "H";
        }
        if (sKey == "fan")
        {
            return "F";
        }
        if (sKey == "battery")
        {
            return LV_SYMBOL_BATTERY_FULL;
        }
        if (sKey == "solar")
        {
            return "S";
        }
        if (sKey == "power")
        {
            return LV_SYMBOL_POWER;
        }
        if (sKey == "thermometer")
        {
            return "T";
        }
        if (sKey == "home")
        {
            return LV_SYMBOL_HOME;
        }
        if (sKey == "wifi")
        {
            return LV_SYMBOL_WIFI;
        }
        if (sKey == "alert")
        {
            return LV_SYMBOL_WARNING;
        }
        if (sKey == "water")
        {
            return "E";
        }
        if (sKey == "gear")
        {
            return LV_SYMBOL_SETTINGS;
        }

        return "?";
    }

    // Keep command values and dynamic thresholds unrounded; only format visible text.
    String FormatWidgetNumber(const String& sValue, JsonObjectConst tWidget)
    {
        JsonVariantConst tDecimals = tWidget["decimal_places"];
        if (tDecimals.isNull() || !tDecimals.is<int>())
        {
            return sValue;
        }
        const int iPlaces = tDecimals.as<int>();
        if (iPlaces < 0 || iPlaces > 2)
        {
            return sValue;
        }
        String sNumeric = sValue;
        sNumeric.trim();
        sNumeric.replace(',', '.');
        if (sNumeric.isEmpty())
        {
            return sValue;
        }
        char* pEnd = nullptr;
        const float fValue = strtof(sNumeric.c_str(), &pEnd);
        if (pEnd == sNumeric.c_str() || *pEnd != '\0' || !isfinite(fValue))
        {
            return sValue;
        }
        return String(fValue, iPlaces);
    }

    String FormatPower(const String& sValue, const String& sUnit)
    {
        return sValue == "--" ? "-- " + sUnit : sValue + " " + sUnit;
    }
}

DashboardRuntime::DashboardRuntime() : oServer(iApiPort), oPortal(80), oProject(iJsonCapacity)
{
}

void DashboardRuntime::loadPreferences()
{
    oPreferences.begin("dashboardesp", false);
    sDeviceName = oPreferences.getString("name", "DashboardESP");
    sWifiSsid = oPreferences.getString("ssid", "");
    sWifiPassword = oPreferences.getString("password", "");
    sAdminToken = oPreferences.getString("token", "");

    if (sAdminToken.length() < 24)
    {
        char acToken[33] = {};
        snprintf(acToken, sizeof(acToken), "%08lx%08lx%08lx%08lx",
                 static_cast<unsigned long>(esp_random()),
                 static_cast<unsigned long>(esp_random()),
                 static_cast<unsigned long>(esp_random()),
                 static_cast<unsigned long>(esp_random()));
        sAdminToken = acToken;
        oPreferences.putString("token", sAdminToken);
    }

    Serial.printf("[DEVICE] %s (%s), API port %u\n", sDeviceName.c_str(), WiFi.macAddress().c_str(), iApiPort);
    Serial.printf("[SECURITY] Local API token: %s\n", sAdminToken.c_str());
    Serial.println("[SECURITY] Keep this token private; local HTTP is not encrypted.");
}

void DashboardRuntime::startAccessPoint()
{
    if (bAccessPoint)
    {
        return;
    }

    const String sName = "DashboardESP-" + String((uint32_t)(ESP.getEfuseMac() & 0xFFFF), HEX);
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(sName.c_str(), sAdminToken.substring(0, 12).c_str());
    oDns.start(53, "*", WiFi.softAPIP());
    oPortal.onNotFound([this]()
    {
        oPortal.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + ":8090/", true);
        oPortal.send(302, "text/plain", "DashboardESP Wi-Fi setup");
    });
    oPortal.begin();
    bAccessPoint = true;
    Serial.printf("[SETUP] AP %s, IP %s; password is first 12 characters of API token\n",
                  sName.c_str(), WiFi.softAPIP().toString().c_str());
}

void DashboardRuntime::startWifi()
{
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(false);
    WiFi.mode(WIFI_STA);

    if (sWifiSsid.isEmpty())
    {
        bSetupMode = true;
        startAccessPoint();
        return;
    }

    WiFi.begin(sWifiSsid.c_str(), sWifiPassword.c_str());
    ulLastWifiAttempt = millis();
    Serial.printf("[WIFI] Connecting to %s\n", sWifiSsid.c_str());
}

bool DashboardRuntime::authorize()
{
    const String sGiven = oServer.header("X-Dashboard-Token");

    if (sGiven.length() != sAdminToken.length())
    {
        oServer.send(401, "application/json", "{\"error\":\"unauthorized\"}");
        return false;
    }

    uint8_t iDifference = 0;

    for (size_t iIndex = 0; iIndex < sAdminToken.length(); iIndex++)
    {
        iDifference |= static_cast<uint8_t>(sGiven[iIndex] ^ sAdminToken[iIndex]);
    }

    if (iDifference != 0)
    {
        oServer.send(401, "application/json", "{\"error\":\"unauthorized\"}");
        return false;
    }

    return true;
}

void DashboardRuntime::sendJson(int iStatus, const String& sJson)
{
    oServer.sendHeader("Cache-Control", "no-store");
    oServer.send(iStatus, "application/json; charset=utf-8", sJson);
}

void DashboardRuntime::registerRoutes()
{
    const char* asHeaders[] = {"X-Dashboard-Token"};
    oServer.collectHeaders(asHeaders, 1);

    oServer.on("/", HTTP_GET, [this]()
    {
        String sHtml = "<!doctype html><html><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
        sHtml += "<title>DashboardESP Setup</title><body style='font-family:sans-serif;max-width:480px;margin:24px auto'>";
        sHtml += "<h2>DashboardESP - Wi-Fi</h2><p>" + HtmlEscape(sDeviceName) + "</p>";
        sHtml += "<form action='/setup' method='POST'><label>SSID<br><input name='ssid' maxlength='32' value='" + HtmlEscape(sWifiSsid) + "'></label><p>";
        sHtml += "<label>Mot de passe<br><input name='password' type='password' maxlength='64'></label></p>";
        sHtml += "<label>Nom de l'ESP<br><input name='name' maxlength='40' value='" + HtmlEscape(sDeviceName) + "'></label><p>";
        sHtml += "<button>Enregistrer et redemarrer</button></p></form>";
        sHtml += "<small>Configuration disponible uniquement pendant le mode initialisation.</small></body></html>";
        oServer.send(200, "text/html; charset=utf-8", sHtml);
    });

    oServer.on("/setup", HTTP_POST, [this]()
    {
        if (!bSetupMode && !bAccessPoint)
        {
            sendJson(403, "{\"error\":\"setup_closed\"}");
            return;
        }

        const String sSsid = oServer.arg("ssid");
        const String sPassword = oServer.arg("password");
        const String sName = oServer.arg("name");

        if (sSsid.isEmpty() || sSsid.length() > 32 || sPassword.length() > 64 || sName.length() > 40)
        {
            sendJson(400, "{\"error\":\"invalid_setup\"}");
            return;
        }

        oPreferences.putString("ssid", sSsid);
        oPreferences.putString("password", sPassword);
        oPreferences.putString("name", sName.isEmpty() ? "DashboardESP" : sName);
        oServer.send(200, "text/plain; charset=utf-8", "Configuration enregistree. Redemarrage.");
        delay(400);
        ESP.restart();
    });

    oServer.on("/api/reset-wifi", HTTP_POST, [this]()
    {
        if (!authorize())
        {
            return;
        }

        bSetupMode = true;
        startAccessPoint();
        showSetup();
        sendJson(200, "{\"ok\":true}");
    });

    oServer.on("/api/status", HTTP_GET, [this]()
    {
        if (!authorize())
        {
            return;
        }

        DynamicJsonDocument oResult(1024);
        oResult["firmware"] = DASH_VERSION;
        oResult["board"] = dash::sBoardName;
        oResult["orientation"] = dash::sOrientationName;
        oResult["resolution"]["width"] = dash::iWidth;
        oResult["resolution"]["height"] = dash::iHeight;
        oResult["device_name"] = sDeviceName;
        oResult["ip"] = WiFi.localIP().toString();
        oResult["rssi"] = WiFi.RSSI();
        oResult["heap"] = ESP.getFreeHeap();
        oResult["uptime_ms"] = millis();
        oResult["current_page"] = sCurrentPage;
        oResult["project_loaded"] = bProjectReady;
        oResult["last_error"] = sLastError;
        String sJson;
        serializeJson(oResult, sJson);
        sendJson(200, sJson);
    });

    oServer.on("/api/config", HTTP_GET, [this]()
    {
        if (!authorize())
        {
            return;
        }

        File oFile = LittleFS.open(sProjectFile, "r");

        if (!oFile)
        {
            sendJson(404, "{\"error\":\"not_found\"}");
            return;
        }

        oServer.streamFile(oFile, "application/json");
        oFile.close();
    });

    oServer.on("/api/config", HTTP_POST, [this]()
    {
        if (!authorize())
        {
            return;
        }

        const String sBody = oServer.arg("plain");

        if (sBody.length() > iMaxConfigBytes)
        {
            sendJson(413, "{\"error\":\"config_too_large\"}");
            return;
        }

        if (!saveProject(sBody))
        {
            DynamicJsonDocument oError(256);
            oError["error"] = sLastError;
            String sJson;
            serializeJson(oError, sJson);
            sendJson(422, sJson);
            return;
        }

        sendJson(200, "{\"ok\":true,\"activated\":true}");
    });

    oServer.on("/api/values", HTTP_POST, [this]()
    {
        if (!authorize())
        {
            return;
        }

        if (oServer.arg("plain").length() > 8192)
        {
            sendJson(413, "{\"error\":\"values_too_large\"}");
            return;
        }

        DynamicJsonDocument oValues(8192);
        const DeserializationError tError = deserializeJson(oValues, oServer.arg("plain"));

        if (tError || !oValues["values"].is<JsonObject>())
        {
            sendJson(400, "{\"error\":\"invalid_values\"}");
            return;
        }

        JsonObjectConst tValues = oValues["values"].as<JsonObjectConst>();

        for (JsonPairConst tPair : tValues)
        {
            const int iCommandId = String(tPair.key().c_str()).toInt();

            if (iCommandId > 0 && !tPair.value().isNull())
            {
                String sValue;
                serializeJson(tPair.value(), sValue);

                if (tPair.value().is<const char*>())
                {
                    sValue = tPair.value().as<String>();
                }

                setValue(iCommandId, sValue);
            }
        }

        updateWidgets();
        sendJson(200, "{\"ok\":true}");
    });

    oServer.on("/api/actions", HTTP_GET, [this]()
    {
        if (!authorize())
        {
            return;
        }

        DynamicJsonDocument oResult(1024);
        JsonArray tActions = oResult.createNestedArray("actions");

        for (int iIndex = 0; iIndex < iActionCount; iIndex++)
        {
            if (atActions[iIndex].bHasValue)
            {
                JsonObject tAction = tActions.createNestedObject();
                tAction["command_id"] = atActions[iIndex].iCommandId;
                tAction["value"] = atActions[iIndex].fValue;
            }
            else
            {
                tActions.add(atActions[iIndex].iCommandId);
            }
        }

        iActionCount = 0;
        String sJson;
        serializeJson(oResult, sJson);
        sendJson(200, sJson);
    });

    oServer.on("/api/page", HTTP_POST, [this]()
    {
        if (!authorize())
        {
            return;
        }

        const String sPageId = oServer.arg("plain");
        JsonArrayConst tPages = oProject["pages"].as<JsonArrayConst>();

        for (JsonObjectConst tPage : tPages)
        {
            if (String(tPage["id"].as<const char*>()) == sPageId)
            {
                renderPage(sPageId);
                sendJson(200, "{\"ok\":true}");
                return;
            }
        }

        sendJson(404, "{\"error\":\"page_not_found\"}");
    });

    oServer.onNotFound([this]()
    {
        if (bAccessPoint)
        {
            oServer.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
            oServer.send(302, "text/plain", "Setup");
        }
        else
        {
            sendJson(404, "{\"error\":\"not_found\"}");
        }
    });

    oServer.begin();
}

void DashboardRuntime::begin()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("[FS] LittleFS mounting failed");
    }

    loadPreferences();
    startWifi();
    registerRoutes();
    configTime(0, 0, "pool.ntp.org", "time.google.com");
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    if (loadProject())
    {
        bSetupMode = false;
        renderPage(sCurrentPage);
    }
    else if (sWifiSsid.isEmpty())
    {
        showSetup();
    }
    else
    {
        showDefault();
    }
}

void DashboardRuntime::handleSerial()
{
    // Read only full CR/LF-terminated commands; never block LVGL or Wi-Fi.
    static String sLine;
    static bool bOverflow = false;
    int iBudget = 96;
    while (Serial.available() > 0 && iBudget-- > 0)
    {
        const char cValue = static_cast<char>(Serial.read());
        if (cValue == '\r' || cValue == '\n')
        {
            if (bOverflow)
            {
                Serial.println("[SERIAL] ERROR: line_too_long");
            }
            else if (!sLine.isEmpty())
            {
                processSerialCommand(sLine);
            }
            sLine = "";
            bOverflow = false;
            continue;
        }
        if (static_cast<uint8_t>(cValue) < 32)
        {
            continue;
        }
        if (sLine.length() >= 191)
        {
            bOverflow = true;
        }
        else if (!bOverflow)
        {
            sLine += cValue;
        }
    }
}

void DashboardRuntime::processSerialCommand(String sCommand)
{
    sCommand.trim();
    if (sCommand.equalsIgnoreCase("info"))
    {
        Serial.printf("[INFO] DashboardESP V%s / %s / %ux%u / LVGL %d.%d\n",
                      DASH_VERSION, dash::sBoardName, dash::iWidth, dash::iHeight,
                      LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR);
        Serial.printf("[INFO] WiFi=%s IP=%s SSID=%s\n",
                      WiFi.status() == WL_CONNECTED ? "CONNECTED" : "OFFLINE",
                      WiFi.localIP().toString().c_str(), sWifiSsid.c_str());
        Serial.printf("[INFO] API_PORT=%u API_KEY=%s\n", iApiPort, sAdminToken.c_str());
        Serial.printf("[INFO] HEAP_FREE=%lu PROJECT=%s\n",
                      static_cast<unsigned long>(ESP.getFreeHeap()), bProjectReady ? "LOADED" : "NONE");
        return;
    }
    if (sCommand.equalsIgnoreCase("help") || sCommand == "?")
    {
        Serial.println("[SERIAL] info<CR> | wifi <ssid> <pwd><CR> | help<CR>");
        Serial.println("[SERIAL] Use quoted SSID/password if they contain spaces.");
        return;
    }
    if (sCommand.substring(0, 5).equalsIgnoreCase("wifi "))
    {
        String sArgs = sCommand.substring(5);
        sArgs.trim();
        String sParts[2];
        size_t iPosition = 0;
        for (int iField = 0; iField < 2; ++iField)
        {
            while (iPosition < sArgs.length() && sArgs[iPosition] == ' ') ++iPosition;
            if (iPosition >= sArgs.length()) break;
            if (sArgs[iPosition] == '"')
            {
                ++iPosition;
                while (iPosition < sArgs.length() && sArgs[iPosition] != '"')
                {
                    sParts[iField] += sArgs[iPosition++];
                }
                if (iPosition < sArgs.length()) ++iPosition;
            }
            else
            {
                while (iPosition < sArgs.length() && sArgs[iPosition] != ' ')
                {
                    sParts[iField] += sArgs[iPosition++];
                }
            }
        }
        while (iPosition < sArgs.length() && sArgs[iPosition] == ' ') ++iPosition;
        if (iPosition != sArgs.length() || sParts[0].isEmpty() ||
            sParts[0].length() > 32 || sParts[1].length() > 63 ||
            (!sParts[1].isEmpty() && sParts[1].length() < 8))
        {
            Serial.println("[WIFI] ERROR syntax: wifi <ssid> <pwd> (password >=8 or quoted empty)");
            return;
        }
        oPreferences.putString("ssid", sParts[0]);
        oPreferences.putString("password", sParts[1]);
        Serial.printf("[WIFI] SAVED SSID=%s; restarting to connect...\n", sParts[0].c_str());
        Serial.flush();
        delay(200);
        ESP.restart();
        return;
    }
    Serial.println("[SERIAL] ERROR unknown_command; type help");
}

void DashboardRuntime::loop()
{
    handleSerial();
    if (!bInfoVisible && !sPendingPage.isEmpty())
    {
        const String sDestination = sPendingPage;
        sPendingPage = "";
        renderPage(sDestination);
    }

    if (bPortalRequested)
    {
        bPortalRequested = false;
        bSetupMode = true;
        startAccessPoint();
        showSetup();
    }

    oServer.handleClient();

    if (bAccessPoint)
    {
        oDns.processNextRequest();
        oPortal.handleClient();
    }

    if (!sWifiSsid.isEmpty() && WiFi.status() != WL_CONNECTED &&
        millis() - ulLastWifiAttempt > ulWifiRetryMs)
    {
        ulLastWifiAttempt = millis();
        WiFi.reconnect();
    }

    if (!sWifiSsid.isEmpty() && WiFi.status() != WL_CONNECTED &&
        millis() > 45000 && !bAccessPoint)
    {
        startAccessPoint();
    }

    if (millis() - ulLastScreenUpdate > ulRefreshMs)
    {
        ulLastScreenUpdate = millis();
        updateWidgets();
        refreshStatus();
    }

    if (millis() - ulLastDiagnostic > 30000)
    {
        ulLastDiagnostic = millis();
        Serial.printf("[HEALTH] wifi=%d ip=%s heap=%lu page=%s\n",
                      static_cast<int>(WiFi.status()), WiFi.localIP().toString().c_str(),
                      static_cast<unsigned long>(ESP.getFreeHeap()), sCurrentPage.c_str());
    }
}

void DashboardRuntime::refreshStatus()
{
    if (pStatus == nullptr)
    {
        return;
    }

    const String sConnection = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "Hors ligne";
    SetText(pStatus, sDeviceName + " | " + sConnection);
}

void DashboardRuntime::addInfoButton(lv_obj_t* pParent)
{
    lv_obj_t* pButton = lv_button_create(pParent);
    lv_obj_set_size(pButton, dash::iWidth >= 480 ? 72 : 48, 34);
    lv_obj_align(pButton, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_obj_set_style_bg_color(pButton, lv_color_hex(0x164A73), 0);
    lv_obj_set_style_radius(pButton, 7, 0);
    lv_obj_add_event_cb(pButton, infoOpenEvent, LV_EVENT_CLICKED, this);

    lv_obj_t* pLabel = lv_label_create(pButton);
    lv_label_set_text(pLabel, dash::iWidth >= 480 ? "INFO" : "i");
    lv_obj_set_style_text_color(pLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(pLabel);
    lv_obj_move_foreground(pButton);
}

void DashboardRuntime::infoOpenEvent(lv_event_t* pEvent)
{
    DashboardRuntime* pSelf = static_cast<DashboardRuntime*>(lv_event_get_user_data(pEvent));
    if (pSelf != nullptr)
    {
        pSelf->bInfoReturnSetup = pSelf->bSetupMode;
        pSelf->bInfoTokenRevealed = false;
        pSelf->showInfo();
    }
}

void DashboardRuntime::infoRevealEvent(lv_event_t* pEvent)
{
    DashboardRuntime* pSelf = static_cast<DashboardRuntime*>(lv_event_get_user_data(pEvent));
    if (pSelf != nullptr)
    {
        pSelf->bInfoTokenRevealed = !pSelf->bInfoTokenRevealed;
        pSelf->showInfo();
    }
}

void DashboardRuntime::infoBackEvent(lv_event_t* pEvent)
{
    DashboardRuntime* pSelf = static_cast<DashboardRuntime*>(lv_event_get_user_data(pEvent));
    if (pSelf == nullptr)
    {
        return;
    }

    pSelf->bInfoVisible = false;
    pSelf->bInfoTokenRevealed = false;
    if (pSelf->bInfoReturnSetup)
    {
        pSelf->showSetup();
    }
    else if (pSelf->bProjectReady)
    {
        pSelf->renderPage(pSelf->sCurrentPage);
    }
    else
    {
        pSelf->showDefault();
    }
}

void DashboardRuntime::infoWifiEvent(lv_event_t* pEvent)
{
    DashboardRuntime* pSelf = static_cast<DashboardRuntime*>(lv_event_get_user_data(pEvent));
    if (pSelf != nullptr)
    {
        pSelf->bInfoVisible = false;
        pSelf->bInfoTokenRevealed = false;
        pSelf->bSetupMode = true;
        pSelf->startAccessPoint();
        pSelf->showSetup();
    }
}

void DashboardRuntime::showInfo()
{
    bInfoVisible = true;
    lv_obj_t* pRoot = lv_screen_active();
    lv_obj_clean(pRoot);
    resetWidgets();
    pScreen = pRoot;
    lv_obj_set_style_bg_color(pRoot, lv_color_hex(0x101827), 0);
    lv_obj_set_style_pad_all(pRoot, 0, 0);
    lv_obj_set_scrollbar_mode(pRoot, LV_SCROLLBAR_MODE_OFF);

    const bool bLarge = dash::iWidth >= 480;
    const int iCardWidth = bLarge ? 650 : dash::iWidth - 16;
    lv_obj_t* pCard = lv_obj_create(pRoot);
    lv_obj_set_size(pCard, iCardWidth, bLarge ? 395 : dash::iHeight - 8);
    lv_obj_align(pCard, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(pCard, lv_color_hex(0x192C44), 0);
    lv_obj_set_style_border_width(pCard, 1, 0);
    lv_obj_set_style_border_color(pCard, lv_color_hex(0x3A668E), 0);
    lv_obj_set_style_radius(pCard, 12, 0);
    lv_obj_set_style_pad_all(pCard, 8, 0);
    lv_obj_set_scrollbar_mode(pCard, LV_SCROLLBAR_MODE_OFF);

    auto fLabel = [&](const String& sText, int iTop, uint32_t ulColor, const lv_font_t* pFont)
    {
        lv_obj_t* pLabel = lv_label_create(pCard);
        lv_label_set_text(pLabel, sText.c_str());
        lv_obj_set_width(pLabel, iCardWidth - 24);
        lv_obj_set_style_text_align(pLabel, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(pLabel, lv_color_hex(ulColor), 0);
        lv_obj_set_style_text_font(pLabel, pFont, 0);
        lv_obj_align(pLabel, LV_ALIGN_TOP_MID, 0, iTop);
        return pLabel;
    };

    fLabel("DASHBOARDESP - INFORMATIONS", 7, 0xFFFFFF, &lv_font_montserrat_20);
    fLabel("Nom : " + sDeviceName, 46, 0xDCEBFF, &lv_font_montserrat_14);
    fLabel("Firmware : V" + String(DASH_VERSION) + "  |  " + String(dash::sBoardName), 62,
           0xA9C8E8, &lv_font_montserrat_12);
    const bool bConnected = WiFi.status() == WL_CONNECTED;
    const String sAddress = bConnected ? WiFi.localIP().toString() :
                            (bAccessPoint ? WiFi.softAPIP().toString() + " (mode AP)" : "Non connecte");
    fLabel("IP : " + sAddress + "    Port : 8090", 91, 0x78D4FF, &lv_font_montserrat_14);
    fLabel("Cle API (X-Dashboard-Token)", 123, 0xCBDCF0, &lv_font_montserrat_14);
    fLabel(bInfoTokenRevealed ? sAdminToken : "********************************", 143,
           0xFDE68A, &lv_font_montserrat_14);
    fLabel("Cle conservee apres redemarrage - ne pas partager", 178, 0xA4BACF, &lv_font_montserrat_12);

    auto fButton = [&](const char* sText, int iOffsetX, int iTop, int iWidth, lv_event_cb_t pCallback)
    {
        lv_obj_t* pButton = lv_button_create(pCard);
        lv_obj_set_size(pButton, iWidth, 43);
        lv_obj_align(pButton, LV_ALIGN_TOP_MID, iOffsetX, iTop);
        lv_obj_add_event_cb(pButton, pCallback, LV_EVENT_CLICKED, this);
        lv_obj_t* pText = lv_label_create(pButton);
        lv_label_set_text(pText, sText);
        lv_obj_center(pText);
    };

    fButton(bInfoTokenRevealed ? "MASQUER LA CLE" : "AFFICHER LA CLE", 0, 212,
            bLarge ? 205 : iCardWidth - 35, infoRevealEvent);
    fButton("RETOUR", bLarge ? -122 : -60, 279, bLarge ? 200 : 110, infoBackEvent);
    fButton("CONFIG. WI-FI", bLarge ? 122 : 60, 279, bLarge ? 200 : 110, infoWifiEvent);
    fLabel("Pour le plugin Jeedom : IP + 8090 + cle API", 343, 0xB4CBDD, &lv_font_montserrat_12);
    Serial.printf("[INFO] IP=%s port=8090 key=%s\n", sAddress.c_str(),
                  bInfoTokenRevealed ? "visible on screen" : "hidden");
}

void DashboardRuntime::showDefault()
{
    lv_obj_clean(lv_screen_active());
    resetWidgets();
    pScreen = lv_screen_active();
    lv_obj_set_style_bg_color(pScreen, lv_color_hex(ulBackground), 0);
    lv_obj_t* pTitle = lv_label_create(pScreen);
    lv_label_set_text(pTitle, "DashboardESP");
    lv_obj_set_style_text_font(pTitle, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(pTitle, lv_color_hex(ulForeground), 0);
    lv_obj_align(pTitle, LV_ALIGN_CENTER, 0, -35);
    pStatus = lv_label_create(pScreen);
    lv_obj_set_style_text_color(pStatus, lv_color_hex(0x96B7E3), 0);
    lv_obj_align(pStatus, LV_ALIGN_CENTER, 0, 8);
    refreshStatus();
    lv_obj_t* pHint = lv_label_create(pScreen);
    lv_label_set_text(pHint, "En attente de configuration JSON");
    lv_obj_set_style_text_color(pHint, lv_color_hex(0xB3C4DC), 0);
    lv_obj_align(pHint, LV_ALIGN_CENTER, 0, 45);
    lv_obj_add_event_cb(pScreen, rootLongPress, LV_EVENT_LONG_PRESSED, this);
    addInfoButton(pScreen);
}

void DashboardRuntime::textareaEvent(lv_event_t* pEvent)
{
    DashboardRuntime* pSelf = static_cast<DashboardRuntime*>(lv_event_get_user_data(pEvent));

    if (pSelf == nullptr || pSelf->pKeyboard == nullptr)
    {
        return;
    }

    lv_obj_t* pInput = static_cast<lv_obj_t*>(lv_event_get_target(pEvent));
    lv_keyboard_set_textarea(pSelf->pKeyboard, pInput);
    lv_obj_remove_flag(pSelf->pKeyboard, LV_OBJ_FLAG_HIDDEN);
}

void DashboardRuntime::rootLongPress(lv_event_t* pEvent)
{
    DashboardRuntime* pSelf = static_cast<DashboardRuntime*>(lv_event_get_user_data(pEvent));

    if (pSelf == nullptr || pSelf->bSetupMode)
    {
        return;
    }

    pSelf->bSetupMode = true;
    pSelf->startAccessPoint();
    pSelf->showSetup();
}

void DashboardRuntime::setupEvent(lv_event_t* pEvent)
{
    DashboardRuntime* pSelf = static_cast<DashboardRuntime*>(lv_event_get_user_data(pEvent));

    if (pSelf == nullptr)
    {
        return;
    }

    const String sSsid = lv_textarea_get_text(pSelf->pWifiSsid);
    const String sPassword = lv_textarea_get_text(pSelf->pWifiPassword);
    const String sName = lv_textarea_get_text(pSelf->pDeviceName);

    if (sSsid.isEmpty() || sSsid.length() > 32 || sPassword.length() > 64 || sName.length() > 40)
    {
        SetText(pSelf->pStatus, "Parametres invalides");
        return;
    }

    pSelf->oPreferences.putString("ssid", sSsid);
    pSelf->oPreferences.putString("password", sPassword);
    pSelf->oPreferences.putString("name", sName);
    SetText(pSelf->pStatus, "Enregistre. Redemarrage...");
    delay(250);
    ESP.restart();
}

void DashboardRuntime::showSetup()
{
    lv_obj_clean(lv_screen_active());
    resetWidgets();
    lv_obj_t* pRoot = lv_screen_active();
    lv_obj_set_style_bg_color(pRoot, lv_color_hex(ulBackground), 0);
    lv_obj_set_scroll_dir(pRoot, LV_DIR_VER);

    lv_obj_t* pTitle = lv_label_create(pRoot);
    lv_label_set_text(pTitle, "DashboardESP V" DASH_VERSION " - Wi-Fi");
    lv_obj_set_style_text_color(pTitle, lv_color_hex(ulForeground), 0);
    lv_obj_align(pTitle, LV_ALIGN_TOP_MID, 0, 4);

    const int iFieldWidth = dash::iWidth > 320 ? 220 : 185;
    const int iTop = 30;
    pWifiSsid = lv_textarea_create(pRoot);
    lv_obj_set_size(pWifiSsid, iFieldWidth, 35);
    lv_obj_align(pWifiSsid, LV_ALIGN_TOP_MID, 0, iTop);
    lv_textarea_set_placeholder_text(pWifiSsid, "Nom du Wi-Fi (SSID)");
    lv_textarea_set_text(pWifiSsid, sWifiSsid.c_str());
    lv_obj_add_event_cb(pWifiSsid, textareaEvent, LV_EVENT_FOCUSED, this);

    pWifiPassword = lv_textarea_create(pRoot);
    lv_obj_set_size(pWifiPassword, iFieldWidth, 35);
    lv_obj_align(pWifiPassword, LV_ALIGN_TOP_MID, 0, iTop + 41);
    lv_textarea_set_placeholder_text(pWifiPassword, "Mot de passe Wi-Fi");
    lv_textarea_set_password_mode(pWifiPassword, true);
    lv_obj_add_event_cb(pWifiPassword, textareaEvent, LV_EVENT_FOCUSED, this);

    pDeviceName = lv_textarea_create(pRoot);
    lv_obj_set_size(pDeviceName, iFieldWidth, 35);
    lv_obj_align(pDeviceName, LV_ALIGN_TOP_MID, 0, iTop + 82);
    lv_textarea_set_text(pDeviceName, sDeviceName.c_str());
    lv_obj_add_event_cb(pDeviceName, textareaEvent, LV_EVENT_FOCUSED, this);

    lv_obj_t* pSave = lv_button_create(pRoot);
    lv_obj_set_size(pSave, 118, 38);
    lv_obj_align(pSave, LV_ALIGN_TOP_MID, 0, iTop + 123);
    lv_obj_add_event_cb(pSave, setupEvent, LV_EVENT_CLICKED, this);
    lv_obj_t* pLabel = lv_label_create(pSave);
    lv_label_set_text(pLabel, "ENREGISTRER");
    lv_obj_center(pLabel);

    pStatus = lv_label_create(pRoot);
    const String sPortal = "Portail: 192.168.4.1:8090";
    lv_label_set_text(pStatus, sPortal.c_str());
    Serial.printf("[SETUP] Wi-Fi AP password: %s\n", sAdminToken.substring(0, 12).c_str());
    lv_obj_set_style_text_color(pStatus, lv_color_hex(0xC0D1E5), 0);
    lv_obj_align(pStatus, LV_ALIGN_TOP_MID, 0, iTop + 170);

    lv_obj_t* pAccess = lv_label_create(pRoot);
    const String sAccess = "AP: DashboardESP-" + String((uint32_t)(ESP.getEfuseMac() & 0xFFFF), HEX) +
                           "  PASS: " + sAdminToken.substring(0, 12);
    lv_label_set_text(pAccess, sAccess.c_str());
    lv_obj_set_style_text_font(pAccess, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(pAccess, lv_color_hex(0xDCE5FF), 0);
    lv_obj_align(pAccess, LV_ALIGN_TOP_MID, 0, iTop + 195);

    pKeyboard = lv_keyboard_create(pRoot);
    lv_obj_set_width(pKeyboard, lv_pct(100));
    lv_obj_set_height(pKeyboard, dash::iHeight > 320 ? 175 : 136);
    lv_obj_align(pKeyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(pKeyboard, LV_OBJ_FLAG_HIDDEN);

    addInfoButton(pRoot);

    // The AP password is not the Wi-Fi network password entered above.
    // It is displayed on the screen during initial setup.
}

void DashboardRuntime::resetWidgets()
{
    iWidgetCount = 0;

    for (int iIndex = 0; iIndex < iMaxWidgetsPerPage; iIndex++)
    {
        atWidgets[iIndex] = WidgetEntry();
    }

    pStatus = nullptr;
    pKeyboard = nullptr;
}

int DashboardRuntime::readCommand(JsonVariantConst tValue)
{
    if (tValue.is<int>())
    {
        return tValue.as<int>();
    }

    if (tValue.is<const char*>())
    {
        return String(tValue.as<const char*>()).toInt();
    }

    return 0;
}

uint32_t DashboardRuntime::parseColor(const String& sColor, uint32_t ulFallback)
{
    if (sColor.length() == 7 && sColor[0] == '#')
    {
        char* pEnd = nullptr;
        const uint32_t ulValue = strtoul(sColor.c_str() + 1, &pEnd, 16);

        if (pEnd != nullptr && *pEnd == '\0')
        {
            return ulValue;
        }
    }

    // The editor may use rgba(...) backgrounds; approximate with a solid RGB color.
    if (sColor.startsWith("rgba("))
    {
        const int iFirst = sColor.indexOf('(');
        const int iSecond = sColor.indexOf(',', iFirst);
        const int iThird = sColor.indexOf(',', iSecond + 1);
        const int iFourth = sColor.indexOf(',', iThird + 1);

        if (iSecond > 0 && iThird > iSecond && iFourth > iThird)
        {
            const uint32_t ulRed = constrain(sColor.substring(iFirst + 1, iSecond).toInt(), 0, 255);
            const uint32_t ulGreen = constrain(sColor.substring(iSecond + 1, iThird).toInt(), 0, 255);
            const uint32_t ulBlue = constrain(sColor.substring(iThird + 1, iFourth).toInt(), 0, 255);
            return (ulRed << 16) | (ulGreen << 8) | ulBlue;
        }
    }

    return ulFallback;
}

bool DashboardRuntime::numericValue(const String& sValue, float& fValue)
{
    String sNumeric = sValue;
    sNumeric.trim();
    sNumeric.replace(',', '.');

    if (sNumeric.isEmpty())
    {
        return false;
    }

    char* pEnd = nullptr;
    fValue = strtof(sNumeric.c_str(), &pEnd);
    return pEnd != sNumeric.c_str() && *pEnd == '\0' && isfinite(fValue);
}

void DashboardRuntime::resolveThreshold(JsonObjectConst tWidget, const String& sValue,
                                        uint32_t& ulText, uint32_t& ulBackground) const
{
    if (!tWidget["dynamicThresholdsEnabled"].as<bool>())
    {
        return;
    }

    float fCurrent = 0;

    if (!numericValue(sValue, fCurrent))
    {
        return;
    }

    JsonArrayConst tBands = tWidget["dynamicThresholdBands"].as<JsonArrayConst>();

    if (tBands.isNull() || tBands.size() > 20)
    {
        return;
    }

    for (JsonObjectConst tBand : tBands)
    {
        if (tBand["max"].isNull() || fCurrent <= tBand["max"].as<float>())
        {
            ulText = parseColor(String(tBand["textColor"] | ""), ulText);
            ulBackground = parseColor(String(tBand["backgroundColor"] | ""), ulBackground);
            return;
        }
    }
}

const lv_font_t* DashboardRuntime::selectFont(int iSize)
{
    // Match the 8..48px editor range with the closest embedded LVGL face.
    // Font faces are in flash (not in the 48KB LVGL heap).
    static const int aiSizes[] = {8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48};
    static const lv_font_t* apFonts[] = {
        &lv_font_montserrat_8, &lv_font_montserrat_10,
        &lv_font_montserrat_12, &lv_font_montserrat_14,
        &lv_font_montserrat_16, &lv_font_montserrat_18,
        &lv_font_montserrat_20, &lv_font_montserrat_22,
        &lv_font_montserrat_24, &lv_font_montserrat_26,
        &lv_font_montserrat_28, &lv_font_montserrat_30,
        &lv_font_montserrat_32, &lv_font_montserrat_34,
        &lv_font_montserrat_36, &lv_font_montserrat_38,
        &lv_font_montserrat_40, &lv_font_montserrat_42,
        &lv_font_montserrat_44, &lv_font_montserrat_46,
        &lv_font_montserrat_48
    };
    iSize = constrain(iSize, 8, 48);
    int iBest = 0;
    for (int iIndex = 1; iIndex < 21; ++iIndex)
    {
        if (abs(aiSizes[iIndex] - iSize) < abs(aiSizes[iBest] - iSize))
        {
            iBest = iIndex;
        }
    }
    return apFonts[iBest];
}

int DashboardRuntime::scaledFont(JsonObjectConst tWidget, int iDefault) const
{
    const int iBase = constrain(static_cast<int>(tWidget["fontSize"] | 16), 8, 48);
    const int iComputed = static_cast<int>(roundf(iDefault * (iBase / 16.0f) * fCanvasScale));
    return constrain(iComputed, 8, 48);
}

void DashboardRuntime::styleWidget(WidgetEntry& tEntry)
{
    JsonObjectConst tWidget = tEntry.tSource;
    const int iX = tWidget["x"] | 0;
    const int iY = tWidget["y"] | 0;
    const int iWidth = tWidget["w"] | 80;
    const int iHeight = tWidget["h"] | 40;
    const int iRadius = tWidget["radius"] | 0;
    const int iFont = tWidget["fontSize"] | 16;
    const uint32_t ulText = parseColor(String(tWidget["color"] | "#ffffff"), ulForeground);
    const uint32_t ulFill = parseColor(String(tWidget["background"] | ""), 0x26416B);

    lv_obj_set_pos(tEntry.pObject,
                   iCanvasOffsetX + static_cast<int>(roundf(iX * fCanvasScale)),
                   iCanvasOffsetY + static_cast<int>(roundf(iY * fCanvasScale)));
    lv_obj_set_size(tEntry.pObject,
                    static_cast<int>(roundf(iWidth * fCanvasScale)),
                    static_cast<int>(roundf(iHeight * fCanvasScale)));
    // LVGL defers its coordinate/layout update. The child positions below must
    // never be calculated using stale coordinates from a newly created object.
    lv_obj_update_layout(tEntry.pObject);
    lv_obj_set_style_radius(tEntry.pObject, max(0, static_cast<int>(roundf(iRadius * fCanvasScale))), 0);
    lv_obj_set_style_bg_opa(tEntry.pObject, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(tEntry.pObject, lv_color_hex(ulFill), 0);
    const String sVisual = tWidget["visual_style"] | "premium";
    const bool bPremium = sVisual != "classic" &&
        (tEntry.sType == "thermostat" || tEntry.sType == "weather" ||
         tEntry.sType == "value" || tEntry.sType == "symbol" ||
         tEntry.sType == "datetime" || tEntry.sType == "energy_summary" ||
         tEntry.sType == "energy_meter" || tEntry.sType == "button" ||
         tEntry.sType == "page_link");
    const uint32_t ulAccent = parseColor(String(tWidget["accent_color"] | ""),
        tEntry.sType == "thermostat" ? 0x2DD4BF :
        (tEntry.sType == "weather" ? 0xFBBF24 : 0x38BDF8));
    lv_obj_set_style_border_width(tEntry.pObject, bPremium ? 1 : 0, 0);
    if (bPremium)
    {
        lv_obj_set_style_border_color(tEntry.pObject, lv_color_hex(ulAccent), 0);
        const String sHalo = tWidget["halo_effect"] | "soft";
        if (sHalo != "off")
        {
            lv_obj_set_style_shadow_color(tEntry.pObject, lv_color_hex(ulAccent), 0);
            lv_obj_set_style_shadow_opa(tEntry.pObject, LV_OPA_40, 0);
            lv_obj_set_style_shadow_width(tEntry.pObject, 8, 0);
            if (sHalo == "pulse")
            {
                lv_anim_t tPulse;
                lv_anim_init(&tPulse);
                lv_anim_set_var(&tPulse, tEntry.pObject);
                lv_anim_set_exec_cb(&tPulse, [](void* pObject, int32_t iValue) {
                    lv_obj_set_style_shadow_width(static_cast<lv_obj_t*>(pObject), iValue, 0);
                });
                lv_anim_set_values(&tPulse, 5, 13);
                lv_anim_set_time(&tPulse, 1450);
                lv_anim_set_playback_time(&tPulse, 1450);
                lv_anim_set_repeat_count(&tPulse, LV_ANIM_REPEAT_INFINITE);
                lv_anim_start(&tPulse);
            }
        }
    }
    lv_obj_set_style_pad_all(tEntry.pObject, 2, 0);
    lv_obj_set_scrollbar_mode(tEntry.pObject, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(tEntry.pObject, LV_OBJ_FLAG_SCROLLABLE);

    if (tEntry.pLabel != nullptr)
    {
        lv_obj_set_width(tEntry.pLabel, lv_pct(100));
        lv_obj_set_style_text_color(tEntry.pLabel, lv_color_hex(ulText), 0);
        lv_obj_set_style_text_font(tEntry.pLabel, selectFont(max(8, static_cast<int>(roundf(iFont * fCanvasScale)))), 0);
        const String sAlign = tWidget["align"] | "center";
        const lv_text_align_t tAlign = sAlign == "left" ? LV_TEXT_ALIGN_LEFT :
                                       (sAlign == "right" ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_CENTER);
        lv_obj_set_style_text_align(tEntry.pLabel, tAlign, 0);
        lv_obj_center(tEntry.pLabel);
    }
}

void DashboardRuntime::queueAction(int iCommandId, bool bHasValue, float fValue)
{
    if (iCommandId <= 0)
    {
        return;
    }

    if (iActionCount >= iMaxActions)
    {
        Serial.println("[ACTION] file pleine");
        return;
    }

    ActionEntry& tAction = atActions[iActionCount++];
    tAction.iCommandId = iCommandId;
    tAction.bHasValue = bHasValue;
    tAction.fValue = fValue;
}

void DashboardRuntime::thermostatModeEvent(lv_event_t* pEvent)
{
    WidgetEntry* pEntry = static_cast<WidgetEntry*>(lv_event_get_user_data(pEvent));

    if (pEntry == nullptr || pEntry->pOwner == nullptr)
    {
        return;
    }

    JsonObjectConst tBindings = pEntry->tSource["thermostat_bindings"].as<JsonObjectConst>();
    lv_obj_t* pTarget = static_cast<lv_obj_t*>(lv_event_get_target(pEvent));
    int iCommandId = 0;

    if (pTarget == pEntry->pModeComfort)
    {
        iCommandId = readCommand(tBindings["mode_comfort"]);
    }
    else if (pTarget == pEntry->pModeEco)
    {
        iCommandId = readCommand(tBindings["mode_eco"]);
    }
    else if (pTarget == pEntry->pModeOff)
    {
        iCommandId = readCommand(tBindings["mode_off"]);
    }

    pEntry->pOwner->queueAction(iCommandId);
}

void DashboardRuntime::thermostatEvent(lv_event_t* pEvent)
{
    WidgetEntry* pEntry = static_cast<WidgetEntry*>(lv_event_get_user_data(pEvent));

    if (pEntry == nullptr || pEntry->pOwner == nullptr || pEntry->iActionId <= 0)
    {
        return;
    }

    DashboardRuntime* pSelf = pEntry->pOwner;
    JsonObjectConst tWidget = pEntry->tSource;
    JsonObjectConst tBindings = tWidget["thermostat_bindings"].as<JsonObjectConst>();
    float fCurrent = pEntry->fThermostatSetpoint;

    if (!isfinite(fCurrent))
    {
        const String sCurrent = pSelf->getBinding(tBindings, "setpoint", "20");

        if (!numericValue(sCurrent, fCurrent))
        {
            fCurrent = 20.0f;
        }
    }

    const float fMinimum = tWidget["thermostat_min"] | 5.0f;
    const float fMaximum = tWidget["thermostat_max"] | 30.0f;
    const float fStep = tWidget["thermostat_step"] | 0.5f;

    if (fStep <= 0.0f || fMaximum <= fMinimum)
    {
        return;
    }

    const lv_obj_t* pTarget = static_cast<lv_obj_t*>(lv_event_get_target(pEvent));
    const float fDirection = pTarget == pEntry->pPlus ? 1.0f : -1.0f;
    const float fValue = roundf((fCurrent + fDirection * fStep) / fStep) * fStep;
    pEntry->fThermostatSetpoint = max(fMinimum, min(fMaximum, fValue));
    pEntry->bThermostatPending = true;
    pSelf->queueAction(pEntry->iActionId, true, pEntry->fThermostatSetpoint);
    pSelf->updateWidget(*pEntry);
}

void DashboardRuntime::widgetEvent(lv_event_t* pEvent)
{
    WidgetEntry* pEntry = static_cast<WidgetEntry*>(lv_event_get_user_data(pEvent));

    if (pEntry == nullptr || pEntry->pOwner == nullptr)
    {
        return;
    }

    DashboardRuntime* pSelf = pEntry->pOwner;

    if (pEntry->sType == "page_link" && !pEntry->sTargetPage.isEmpty())
    {
        pSelf->sPendingPage = pEntry->sTargetPage;
    }
    else if (pEntry->iActionId > 0)
    {
        pSelf->queueAction(pEntry->iActionId);
        Serial.printf("[ACTION] queued command=%d\n", pEntry->iActionId);
    }
}

void DashboardRuntime::renderWidget(JsonObjectConst tWidget)
{
    if (iWidgetCount >= iMaxWidgetsPerPage)
    {
        return;
    }

    WidgetEntry& tEntry = atWidgets[iWidgetCount++];
    tEntry.pOwner = this;
    tEntry.tSource = tWidget;
    tEntry.sId = String(tWidget["id"] | "");
    tEntry.sType = String(tWidget["type"] | "label");
    tEntry.sTargetPage = String(tWidget["target_page_id"] | "");
    tEntry.iInfoId = readCommand(tWidget["command_id"]);
    tEntry.iActionId = tEntry.sType == "symbol" ? readCommand(tWidget["symbol_action_id"]) :
                       (tEntry.sType == "button" ? tEntry.iInfoId : 0);

    if (tEntry.sType == "thermostat")
    {
        JsonObjectConst tBindings = tWidget["thermostat_bindings"].as<JsonObjectConst>();
        tEntry.iActionId = readCommand(tBindings["setpoint_action"]);
    }

    const bool bButton = tEntry.sType == "button" || tEntry.sType == "page_link" ||
                         (tEntry.sType == "symbol" && tEntry.iActionId > 0);
    tEntry.pObject = bButton ? lv_button_create(pScreen) : lv_obj_create(pScreen);
    tEntry.pLabel = lv_label_create(tEntry.pObject);
    lv_label_set_long_mode(tEntry.pLabel, LV_LABEL_LONG_CLIP);
    styleWidget(tEntry);

    if (bButton)
    {
        lv_obj_add_event_cb(tEntry.pObject, widgetEvent, LV_EVENT_CLICKED, &tEntry);
    }

    if (tEntry.sType == "energy_summary")
    {
        const int iWidth = tWidget["w"] | 240;
        const int iHeight = tWidget["h"] | 132;
        const int iCellWidth = max(30, (iWidth - 12) / 2);
        const int iCellHeight = max(24, (iHeight - 30) / 2);
        lv_obj_set_height(tEntry.pLabel, 20);
        lv_obj_align(tEntry.pLabel, LV_ALIGN_TOP_MID, 0, 2);

        for (int iIndex = 0; iIndex < 4; iIndex++)
        {
            lv_obj_t* pCell = lv_obj_create(tEntry.pObject);
            lv_obj_set_size(pCell, iCellWidth, iCellHeight);
            lv_obj_set_pos(pCell, 2 + (iIndex % 2) * (iCellWidth + 3),
                           23 + (iIndex / 2) * (iCellHeight + 2));
            lv_obj_set_style_bg_color(pCell, lv_color_hex(0x2A4364), 0);
            lv_obj_set_style_bg_opa(pCell, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(pCell, 0, 0);
            lv_obj_set_style_radius(pCell, 8, 0);
            lv_obj_set_style_pad_all(pCell, 2, 0);
            lv_obj_remove_flag(pCell, LV_OBJ_FLAG_SCROLLABLE);
            tEntry.apMetricLabels[iIndex] = lv_label_create(pCell);
            lv_obj_set_width(tEntry.apMetricLabels[iIndex], lv_pct(100));
            lv_obj_set_style_text_font(tEntry.apMetricLabels[iIndex], selectFont(scaledFont(tWidget, 12)), 0);
            lv_obj_set_style_text_color(tEntry.apMetricLabels[iIndex], lv_color_hex(0xECF4FF), 0);
            lv_obj_set_style_text_align(tEntry.apMetricLabels[iIndex], LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_center(tEntry.apMetricLabels[iIndex]);
        }
    }
    else if (tEntry.sType == "symbol")
    {
        lv_obj_set_height(tEntry.pLabel, 30);
        lv_obj_align(tEntry.pLabel, LV_ALIGN_CENTER, 0, -10);
        tEntry.pAux = lv_label_create(tEntry.pObject);
        lv_obj_set_width(tEntry.pAux, lv_pct(100));
        lv_obj_set_style_text_color(tEntry.pAux, lv_color_hex(ulForeground), 0);
        lv_obj_set_style_text_font(tEntry.pAux, selectFont(scaledFont(tWidget, 12)), 0);
        lv_obj_set_style_text_align(tEntry.pAux, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(tEntry.pAux, LV_ALIGN_BOTTOM_MID, 0, -2);
    }

    // V0.3.7: bandeau meteo compose, pas de label multi-ligne dans le firmware.
    if (tEntry.sType == "weather" && String(tWidget["visual_style"] | "premium") != "classic")
    {
        const int iWidth = max(1, static_cast<int>(roundf((tWidget["w"] | 140) * fCanvasScale)));
        const int iHeight = max(1, static_cast<int>(roundf((tWidget["h"] | 42) * fCanvasScale)));
        const int iIconWidth = max(25, min(68, iWidth / 4));
        lv_obj_set_style_pad_all(tEntry.pObject, 0, 0);
        lv_obj_set_pos(tEntry.pLabel, iIconWidth + 5, 3);
        lv_obj_set_size(tEntry.pLabel, max(30, iWidth - iIconWidth - 11), max(20, iHeight / 2));
        lv_obj_set_style_text_align(tEntry.pLabel, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_text_color(tEntry.pLabel, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(tEntry.pLabel, selectFont(scaledFont(tWidget, iHeight >= 80 ? 24 : 20)), 0);
        tEntry.pWeatherIcon = lv_label_create(tEntry.pObject);
        lv_obj_set_pos(tEntry.pWeatherIcon, 4, max(1, (iHeight - 30) / 2));
        lv_obj_set_width(tEntry.pWeatherIcon, iIconWidth);
        lv_obj_set_style_text_align(tEntry.pWeatherIcon, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(tEntry.pWeatherIcon, selectFont(scaledFont(tWidget, 24)), 0);
        lv_obj_set_style_text_color(tEntry.pWeatherIcon, lv_color_hex(0xFDE68A), 0);
        tEntry.pWeatherInfo = lv_label_create(tEntry.pObject);
        lv_obj_set_pos(tEntry.pWeatherInfo, iIconWidth + 5, iHeight / 2);
        lv_obj_set_size(tEntry.pWeatherInfo, max(30, iWidth - iIconWidth - 11), max(14, iHeight / 2 - 4));
        lv_obj_set_style_text_align(tEntry.pWeatherInfo, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_text_font(tEntry.pWeatherInfo, selectFont(scaledFont(tWidget, iHeight >= 75 ? 16 : 12)), 0);
        lv_obj_set_style_text_color(tEntry.pWeatherInfo, lv_color_hex(0xFDE68A), 0);
        lv_label_set_long_mode(tEntry.pWeatherInfo, LV_LABEL_LONG_CLIP);
    }

    // V0.3.5: thermostat LVGL natif, structure conforme a l'editeur Studio.
    // Tous les enfants sont places dans les dimensions de la tuile en pixels ecran.
    if (tEntry.sType == "thermostat")
    {
        const int iWidth = max(1, static_cast<int>(roundf((tWidget["w"] | 140) * fCanvasScale)));
        const int iHeight = max(1, static_cast<int>(roundf((tWidget["h"] | 42) * fCanvasScale)));
        const bool bShowModes = tWidget["thermostat_show_modes"] | false;
        const bool bShowControls = tWidget["thermostat_show_controls"] | true;
        const int iPad = iWidth < 170 ? 3 : 5;
        const int iGap = 4;
        const int iTitleHeight = iHeight < 115 ? 17 : 21;
        const int iModeHeight = bShowModes ? (iHeight < 125 ? 19 : 23) : 0;
        const int iFooterHeight = iHeight < 125 ? 23 : 29;
        const int iValueTop = iTitleHeight + 3;
        const int iValueHeight = max(21, iHeight - iValueTop - iFooterHeight - iModeHeight - 3 * iPad);
        const int iCellWidth = max(30, (iWidth - 2 * iPad - iGap) / 2);
        const int iFooterTop = iValueTop + iValueHeight + 2;
        const int iSmallFont = iWidth < 205 ? 12 : 14;
        const int iButtonSize = iWidth < 180 ? 23 : 27;
        Serial.printf("[LAYOUT] thermostat id=%s: %dx%d, valueTop=%d, valueHeight=%d, footer=%d, modes=%d\n",
                      tEntry.sId.c_str(), iWidth, iHeight, iValueTop, iValueHeight, iFooterTop, iModeHeight);

        lv_obj_set_style_pad_all(tEntry.pObject, 0, 0);
        lv_obj_set_style_pad_row(tEntry.pObject, 0, 0);
        lv_obj_set_style_pad_column(tEntry.pObject, 0, 0);
        lv_obj_set_style_text_color(tEntry.pObject, lv_color_hex(0x4ADE80), 0);
        lv_obj_set_size(tEntry.pLabel, iWidth - 2 * iPad, iTitleHeight);
        lv_obj_set_style_text_font(tEntry.pLabel, selectFont(scaledFont(tWidget, iWidth < 190 ? 12 : 16)), 0);
        lv_obj_set_style_text_align(tEntry.pLabel, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(tEntry.pLabel, LV_ALIGN_TOP_LEFT, iPad, 2);

        for (int iIndex = 0; iIndex < 2; ++iIndex)
        {
            lv_obj_t* pCell = lv_obj_create(tEntry.pObject);
            lv_obj_set_pos(pCell, iPad + iIndex * (iCellWidth + iGap), iValueTop);
            lv_obj_set_size(pCell, iCellWidth, iValueHeight);
            lv_obj_set_style_pad_all(pCell, 1, 0);
            lv_obj_set_style_radius(pCell, 8, 0);
            lv_obj_set_style_border_width(pCell, 0, 0);
            lv_obj_set_style_bg_color(pCell, lv_color_hex(0x084755), 0);
            lv_obj_set_style_bg_opa(pCell, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(pCell, 1, 0);
            lv_obj_set_style_border_color(pCell, lv_color_hex(0x268A89), 0);
            lv_obj_remove_flag(pCell, LV_OBJ_FLAG_SCROLLABLE);
            tEntry.apMetricLabels[iIndex] = lv_label_create(pCell);
            lv_obj_set_width(tEntry.apMetricLabels[iIndex], iCellWidth - 4);
            lv_obj_set_style_text_align(tEntry.apMetricLabels[iIndex], LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_style_text_color(tEntry.apMetricLabels[iIndex], lv_color_hex(0x8AFFB8), 0);
            lv_obj_set_style_text_font(tEntry.apMetricLabels[iIndex], selectFont(scaledFont(tWidget, iSmallFont)), 0);
            lv_label_set_long_mode(tEntry.apMetricLabels[iIndex], LV_LABEL_LONG_CLIP);
            lv_obj_center(tEntry.apMetricLabels[iIndex]);
        }

        tEntry.pAux = lv_label_create(tEntry.pObject);
        lv_obj_set_pos(tEntry.pAux, iPad, iFooterTop + 4);
        lv_obj_set_size(tEntry.pAux, max(24, iWidth - 2 * iPad - (bShowControls ? 2 * (iButtonSize + 3) : 0)), iFooterHeight - 2);
        lv_obj_set_style_text_color(tEntry.pAux, lv_color_hex(0xCBD5E1), 0);
        lv_obj_set_style_text_font(tEntry.pAux, selectFont(scaledFont(tWidget, iSmallFont)), 0);
        lv_label_set_long_mode(tEntry.pAux, LV_LABEL_LONG_CLIP);

        JsonObjectConst tBindings = tWidget["thermostat_bindings"].as<JsonObjectConst>();
        if (bShowControls)
        {
            tEntry.pMinus = lv_button_create(tEntry.pObject);
            tEntry.pPlus = lv_button_create(tEntry.pObject);
            lv_obj_t* apControls[2] = {tEntry.pMinus, tEntry.pPlus};
            const char* asLabels[2] = {"-", "+"};
            for (int iIndex = 0; iIndex < 2; ++iIndex)
            {
                lv_obj_t* pControl = apControls[iIndex];
                lv_obj_set_size(pControl, iButtonSize, iButtonSize);
                lv_obj_set_pos(pControl, iWidth - iPad - (2 - iIndex) * (iButtonSize + 3) + 3, iFooterTop);
                lv_obj_set_style_radius(pControl, 5, 0);
                lv_obj_set_style_pad_all(pControl, 0, 0);
                lv_obj_set_style_bg_color(pControl, lv_color_hex(0x24549E), 0);
                lv_obj_t* pText = lv_label_create(pControl);
                lv_label_set_text(pText, asLabels[iIndex]);
                lv_obj_set_style_text_font(pText, selectFont(scaledFont(tWidget, 16)), 0);
                lv_obj_center(pText);
                if (tEntry.iActionId > 0)
                {
                    lv_obj_add_event_cb(pControl, thermostatEvent, LV_EVENT_CLICKED, &tEntry);
                }
                else
                {
                    lv_obj_add_state(pControl, LV_STATE_DISABLED);
                }
            }
        }

        if (bShowModes)
        {
            lv_obj_t** apModes[3] = {&tEntry.pModeComfort, &tEntry.pModeEco, &tEntry.pModeOff};
            const char* asKeys[3] = {"mode_comfort", "mode_eco", "mode_off"};
            const char* asLabels[3] = {"CONFORT", "ECO", "OFF"};
            const int iModeWidth = max(25, (iWidth - 2 * iPad - 2 * iGap) / 3);
            const int iModeY = max(0, iHeight - iModeHeight - iPad);
            for (int iIndex = 0; iIndex < 3; ++iIndex)
            {
                lv_obj_t* pMode = lv_button_create(tEntry.pObject);
                *apModes[iIndex] = pMode;
                lv_obj_set_pos(pMode, iPad + iIndex * (iModeWidth + iGap), iModeY);
                lv_obj_set_size(pMode, iModeWidth, iModeHeight);
                lv_obj_set_style_pad_all(pMode, 0, 0);
                lv_obj_set_style_radius(pMode, 4, 0);
                lv_obj_set_style_bg_color(pMode, lv_color_hex(0x23508B), 0);
                lv_obj_t* pCaption = lv_label_create(pMode);
                lv_label_set_text(pCaption, asLabels[iIndex]);
                lv_obj_set_style_text_font(pCaption, selectFont(scaledFont(tWidget, 12)), 0);
                lv_obj_center(pCaption);
                if (readCommand(tBindings[asKeys[iIndex]]) > 0)
                {
                    lv_obj_add_event_cb(pMode, thermostatModeEvent, LV_EVENT_CLICKED, &tEntry);
                }
                else
                {
                    lv_obj_add_state(pMode, LV_STATE_DISABLED);
                }
            }
        }
    }

    if (tEntry.sType == "energy_meter")
    {
        tEntry.pBar = lv_bar_create(tEntry.pObject);
        lv_obj_set_width(tEntry.pBar, lv_pct(85));
        lv_obj_set_height(tEntry.pBar, 7);
        lv_obj_align(tEntry.pBar, LV_ALIGN_BOTTOM_MID, 0, -5);
        lv_bar_set_range(tEntry.pBar, 0, 100);
    }

    updateWidget(tEntry);
    lv_obj_update_layout(tEntry.pObject);
}

void DashboardRuntime::renderPage(const String& sPageId)
{
    JsonArrayConst tPages = oProject["pages"].as<JsonArrayConst>();

    for (JsonObjectConst tPage : tPages)
    {
        if (String(tPage["id"] | "") != sPageId)
        {
            continue;
        }

        // Preserve the editor's logical coordinates when the ESP has a different
        // resolution. Uniform fit avoids stretching circles and controls.
        const int iSourceWidth = oProject["resolution"]["width"] | dash::iWidth;
        const int iSourceHeight = oProject["resolution"]["height"] | dash::iHeight;
        const int iPhysicalWidth = dash::iWidth;
        const int iPhysicalHeight = dash::iHeight;
        fCanvasScale = 1.0f;
        iCanvasOffsetX = 0;
        iCanvasOffsetY = 0;

        if (iSourceWidth > 0 && iSourceHeight > 0)
        {
            const float fWidthScale = static_cast<float>(iPhysicalWidth) / iSourceWidth;
            const float fHeightScale = static_cast<float>(iPhysicalHeight) / iSourceHeight;
            fCanvasScale = fminf(fWidthScale, fHeightScale);
            iCanvasOffsetX = static_cast<int>(roundf((iPhysicalWidth - iSourceWidth * fCanvasScale) / 2.0f));
            iCanvasOffsetY = static_cast<int>(roundf((iPhysicalHeight - iSourceHeight * fCanvasScale) / 2.0f));
        }

        Serial.printf("[CANVAS] %dx%d -> %dx%d, echelle=%.3f, marge=%d,%d\n",
                      iSourceWidth, iSourceHeight, iPhysicalWidth, iPhysicalHeight,
                      fCanvasScale, iCanvasOffsetX, iCanvasOffsetY);
        lv_obj_clean(lv_screen_active());
        resetWidgets();
        pScreen = lv_screen_active();
        lv_obj_set_style_pad_all(pScreen, 0, 0);
        lv_obj_set_style_bg_color(pScreen, lv_color_hex(parseColor(String(tPage["background"] | "#101827"), ulBackground)), 0);
        lv_obj_set_scrollbar_mode(pScreen, LV_SCROLLBAR_MODE_OFF);
        sCurrentPage = sPageId;
        JsonArrayConst tWidgets = tPage["widgets"].as<JsonArrayConst>();

        for (JsonObjectConst tWidget : tWidgets)
        {
            renderWidget(tWidget);
        }

        addInfoButton(pScreen);
        Serial.printf("[PAGE] %s widgets=%d\n", sPageId.c_str(), iWidgetCount);
        return;
    }

    Serial.printf("[PAGE] missing: %s\n", sPageId.c_str());
}

void DashboardRuntime::setValue(int iCommandId, const String& sValue)
{
    for (int iIndex = 0; iIndex < iValueCount; iIndex++)
    {
        if (atValues[iIndex].iCommandId == iCommandId)
        {
            atValues[iIndex].sValue = sValue.substring(0, 96);
            atValues[iIndex].bValid = true;
            return;
        }
    }

    if (iValueCount < iMaxValues)
    {
        ValueEntry& tEntry = atValues[iValueCount++];
        tEntry.iCommandId = iCommandId;
        tEntry.sValue = sValue.substring(0, 96);
        tEntry.bValid = true;
    }
}

String DashboardRuntime::getValue(int iCommandId, const String& sFallback) const
{
    if (iCommandId <= 0)
    {
        return sFallback;
    }

    for (int iIndex = 0; iIndex < iValueCount; iIndex++)
    {
        if (atValues[iIndex].iCommandId == iCommandId && atValues[iIndex].bValid)
        {
            return atValues[iIndex].sValue;
        }
    }

    return "--";
}

String DashboardRuntime::getBinding(JsonObjectConst tBindings, const char* sKey, const String& sFallback) const
{
    return getValue(readCommand(tBindings[sKey]), sFallback);
}

void DashboardRuntime::updateWidget(WidgetEntry& tEntry)
{
    JsonObjectConst tWidget = tEntry.tSource;
    String sText = String(tWidget["text"] | "");
    uint32_t ulText = parseColor(String(tWidget["color"] | "#ffffff"), ulForeground);
    uint32_t ulFill = parseColor(String(tWidget["background"] | ""), 0x26416B);

    if (tEntry.sType == "datetime")
    {
        time_t tNow = time(nullptr);
        struct tm tLocal = {};
        localtime_r(&tNow, &tLocal);
        char acDate[48] = {};
        const String sMode = tWidget["datetime_mode"] | "datetime";
        const char* sPattern = sMode == "date" ? "%d/%m/%Y" :
                               (sMode == "time" ? "%H:%M" : "%d/%m/%Y %H:%M");

        if (tLocal.tm_year > 120)
        {
            strftime(acDate, sizeof(acDate), sPattern, &tLocal);
            sText = acDate;
        }
        else
        {
            sText = "--/--/---- --:--";
        }
    }
    else if (tEntry.sType == "value")
    {
        const String sValue = getValue(tEntry.iInfoId, sText);
        sText = FormatWidgetNumber(sValue, tWidget);
        if ((tWidget["show_unit"] | false))
        {
            const String sUnit = String(tWidget["unit"] | "");
            if (!sUnit.isEmpty() && !sText.endsWith(sUnit))
            {
                sText += " " + sUnit;
            }
        }
        // Les seuils sont toujours evalues sur la valeur brute, sans unite.
        resolveThreshold(tWidget, sValue, ulText, ulFill);
    }
    else if (tEntry.sType == "thermostat")
    {
        JsonObjectConst tBindings = tWidget["thermostat_bindings"].as<JsonObjectConst>();
        const String sTemperature = getBinding(tBindings, "temperature", "20.8");
        const String sSetpointInfo = getBinding(tBindings, "setpoint", "21");
        float fReported = NAN;
        if (tEntry.bThermostatPending && numericValue(sSetpointInfo, fReported) &&
            fabsf(fReported - tEntry.fThermostatSetpoint) < 0.05f)
        {
            tEntry.bThermostatPending = false;
        }
        String sMode = getBinding(tBindings, "mode", "Confort");
        const String sHeating = getBinding(tBindings, "heating", "0");
        const String sSetpoint = tEntry.bThermostatPending ? String(tEntry.fThermostatSetpoint, 1) : sSetpointInfo;
        const bool bHeating = sHeating.toFloat() > 0.0f || sHeating.equalsIgnoreCase("on") ||
                              sHeating.equalsIgnoreCase("true");

        // LVGL Montserrat embarque peut ne pas contenir les caracteres accentues.
        // Garder des libelles ASCII pour eviter les rectangles dans les modes Jeedom.
        sMode.replace("é", "e");
        sMode.replace("è", "e");
        sMode.replace("ê", "e");
        sMode.replace("É", "E");
        sMode.replace("ê", "e");
        sMode.replace("à", "a");
        sMode.replace("ô", "o");
        sMode.replace("û", "u");
        const String sDegree = String("\xC2\xB0") + "C";
        SetText(tEntry.pLabel, String(tWidget["text"] | "Thermostat"));
        SetText(tEntry.apMetricLabels[0], "Ambiante\n" + FormatWidgetNumber(sTemperature, tWidget) + " " + sDegree);
        SetText(tEntry.apMetricLabels[1], "Consigne\n" + FormatWidgetNumber(sSetpoint, tWidget) + " " + sDegree);
        SetText(tEntry.pAux, String(bHeating ? "Chauffe " : "Mode ") + sMode);
        lv_obj_set_style_text_color(tEntry.pAux, lv_color_hex(bHeating ? 0xFB923C : 0xCBD5E1), 0);
        String sModeLower = sMode;
        sModeLower.toLowerCase();
        const lv_color_t tActiveColor = lv_color_hex(0x047857);
        const lv_color_t tInactiveColor = lv_color_hex(0x23508B);
        if (tEntry.pModeComfort != nullptr)
            lv_obj_set_style_bg_color(tEntry.pModeComfort, sModeLower.indexOf("confort") >= 0 ? tActiveColor : tInactiveColor, 0);
        if (tEntry.pModeEco != nullptr)
            lv_obj_set_style_bg_color(tEntry.pModeEco, sModeLower.indexOf("eco") >= 0 ? tActiveColor : tInactiveColor, 0);
        if (tEntry.pModeOff != nullptr)
            lv_obj_set_style_bg_color(tEntry.pModeOff, (sModeLower.indexOf("arret") >= 0 || sModeLower.indexOf("off") >= 0) ? tActiveColor : tInactiveColor, 0);
        resolveThreshold(tWidget, sTemperature, ulText, ulFill);
        lv_obj_set_style_text_color(tEntry.pLabel, lv_color_hex(ulText), 0);
        lv_obj_set_style_bg_color(tEntry.pObject, lv_color_hex(ulFill), 0);
        return; // Ne jamais remplacer le titre par un bloc texte multiligne.
    }
    else if (tEntry.sType == "weather")
    {
        JsonObjectConst tBindings = tWidget["weather_bindings"].as<JsonObjectConst>();
        const String sTemperature = getBinding(tBindings, "temperature", "21.5");
        const String sCondition = getBinding(tBindings, "condition", "Ensoleille");
        const String sHumidity = getBinding(tBindings, "humidity", "55");
        if (tEntry.pWeatherIcon != nullptr)
        {
            String sLower = sCondition;
            sLower.toLowerCase();
            // Use ASCII fallback when a glyph is absent from LVGL Montserrat.
            SetText(tEntry.pWeatherIcon, sLower.indexOf("pluie") >= 0 ? "///" :
                         (sLower.indexOf("nuage") >= 0 ? "(o)" : "*"));
            SetText(tEntry.pWeatherInfo, sCondition + "  |  " + FormatWidgetNumber(sHumidity, tWidget) + " %");
            SetText(tEntry.pLabel, FormatWidgetNumber(sTemperature, tWidget) + " " + String("\xC2\xB0") + "C");
            resolveThreshold(tWidget, sTemperature, ulText, ulFill);
            lv_obj_set_style_bg_color(tEntry.pObject, lv_color_hex(ulFill), 0);
            return;
        }
        sText = FormatWidgetNumber(sTemperature, tWidget) + " C\n" + sCondition + " | " + FormatWidgetNumber(sHumidity, tWidget) + " %";
        resolveThreshold(tWidget, sTemperature, ulText, ulFill);
    }
    else if (tEntry.sType == "symbol")
    {
        const String sValue = getValue(tEntry.iInfoId, String(tWidget["preview_symbol_value"] | "0"));
        float fValue = 0;
        const bool bActive = numericValue(sValue, fValue) && fValue > (tWidget["symbol_on_above"] | 0.0f);
        const String sSymbolKey = bActive && !String(tWidget["symbol_on_key"] | "").isEmpty() ?
                                  String(tWidget["symbol_on_key"] | "") : String(tWidget["symbol_key"] | "home");
        ulText = parseColor(String(bActive ? (tWidget["symbol_on_color"] | "#4ade80") :
                                    (tWidget["symbol_off_color"] | "#94a3b8")), ulText);
        resolveThreshold(tWidget, sValue, ulText, ulFill);
        const bool bHide = (tWidget["symbol_hide_off"] | false) && !bActive;
        if (bHide)
        {
            lv_obj_add_flag(tEntry.pObject, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_remove_flag(tEntry.pObject, LV_OBJ_FLAG_HIDDEN);
        }

        SetText(tEntry.pAux, sText.isEmpty() ? "Symbole" : sText);
        sText = String(SymbolText(sSymbolKey));
    }
    else if (tEntry.sType == "energy_summary")
    {
        JsonObjectConst tBindings = tWidget["energy_bindings"].as<JsonObjectConst>();
        const char* asKeys[4] = {"solar", "home", "battery", "grid"};
        const char* asLabels[4] = {"Solaire", "Maison", "Batterie", "Reseau"};
        const char* asFallbacks[4] = {"2450", "1250", "600", "-600"};

        for (int iIndex = 0; iIndex < 4; iIndex++)
        {
            const String sMetric = getBinding(tBindings, asKeys[iIndex], asFallbacks[iIndex]);
            SetText(tEntry.apMetricLabels[iIndex], String(asLabels[iIndex]) + "\n" + FormatPower(FormatWidgetNumber(sMetric, tWidget), "W"));
        }
    }
    else if (tEntry.sType == "energy_meter")
    {
        JsonObjectConst tBindings = tWidget["energy_bindings"].as<JsonObjectConst>();
        const String sPower = getBinding(tBindings, "power", "1250");
        sText += "\n" + FormatPower(FormatWidgetNumber(sPower, tWidget), "W");

        if (String(tWidget["energy_mode"] | "mono") == "tri")
        {
            sText += "\nL1 " + FormatWidgetNumber(getBinding(tBindings, "phase1", "420"), tWidget);
            sText += " L2 " + FormatWidgetNumber(getBinding(tBindings, "phase2", "410"), tWidget);
            sText += " L3 " + FormatWidgetNumber(getBinding(tBindings, "phase3", "420"), tWidget);
        }
        else
        {
            sText += "\n" + FormatWidgetNumber(getBinding(tBindings, "voltage", "230"), tWidget) + " V";
            sText += "  " + FormatWidgetNumber(getBinding(tBindings, "current", "5.4"), tWidget) + " A";
            sText += "\n" + FormatWidgetNumber(getBinding(tBindings, "energy", "12.6"), tWidget) + " kWh";
        }

        float fPower = 0;
        const float fMax = max(100.0f, tWidget["energy_max"] | 5000.0f);
        const float fPercent = numericValue(sPower, fPower) ? min(100.0f, fabsf(fPower) * 100.0f / fMax) : 0.0f;

        if (tEntry.pBar != nullptr)
        {
            lv_bar_set_value(tEntry.pBar, static_cast<int>(fPercent), LV_ANIM_OFF);
        }

        resolveThreshold(tWidget, sPower, ulText, ulFill);
    }

    SetText(tEntry.pLabel, sText);
    lv_obj_set_style_text_color(tEntry.pLabel, lv_color_hex(ulText), 0);
    lv_obj_set_style_bg_color(tEntry.pObject, lv_color_hex(ulFill), 0);
}

void DashboardRuntime::updateWidgets()
{
    for (int iIndex = 0; iIndex < iWidgetCount; iIndex++)
    {
        updateWidget(atWidgets[iIndex]);
    }
}

bool DashboardRuntime::validateProject(JsonVariantConst tProject, String& sError)
{
    if (String(tProject["schema"] | "") != "dashboardesp/1")
    {
        sError = "unsupported_schema";
        return false;
    }

    JsonArrayConst tPages = tProject["pages"].as<JsonArrayConst>();

    if (tPages.isNull() || tPages.size() < 1 || tPages.size() > iMaxPages)
    {
        sError = "invalid_page_count";
        return false;
    }

    const int iWidth = tProject["resolution"]["width"] | 0;
    const int iHeight = tProject["resolution"]["height"] | 0;

    if (iWidth != dash::iWidth || iHeight != dash::iHeight)
    {
        sError = "resolution_mismatch";
        return false;
    }

    for (JsonObjectConst tPage : tPages)
    {
        const String sPageId = tPage["id"] | "";
        JsonArrayConst tWidgets = tPage["widgets"].as<JsonArrayConst>();

        if (sPageId.isEmpty() || tWidgets.isNull() || tWidgets.size() > iMaxWidgetsPerPage)
        {
            sError = "invalid_page_or_widgets";
            return false;
        }

        for (JsonObjectConst tWidget : tWidgets)
        {
            const String sType = tWidget["type"] | "";
            const String sId = tWidget["id"] | "";
            const int iX = tWidget["x"] | -1;
            const int iY = tWidget["y"] | -1;
            const int iW = tWidget["w"] | 0;
            const int iH = tWidget["h"] | 0;
            const bool bSupported = sType == "label" || sType == "value" || sType == "button" ||
                                    sType == "rectangle" || sType == "page_link" || sType == "datetime" ||
                                    sType == "weather" || sType == "energy_summary" ||
                                    sType == "energy_meter" || sType == "symbol" || sType == "thermostat";

            if (!bSupported || sId.isEmpty() || iX < 0 || iY < 0 || iW <= 0 || iH <= 0 ||
                iX + iW > iWidth || iY + iH > iHeight)
            {
                sError = "invalid_widget_geometry_or_type";
                return false;
            }
        }
    }

    return true;
}

bool DashboardRuntime::loadProject()
{
    File oFile = LittleFS.open(sProjectFile, "r");

    if (!oFile)
    {
        sLastError = "no_project";
        return false;
    }

    const size_t iFileBytes = oFile.size();
    if (iFileBytes == 0 || iFileBytes > iMaxConfigBytes)
    {
        oFile.close();
        sLastError = "invalid_size";
        return false;
    }

    // Retain the bytes for the entire lifetime of the parsed document.
    // Do not parse from a temporary String or a temporary char buffer.
    sProjectJsonBacking = oFile.readString();
    oFile.close();
    if (sProjectJsonBacking.length() != iFileBytes)
    {
        sLastError = "storage_read_failed";
        return false;
    }

    oProject.clear();
    if (oProject.capacity() == 0)
    {
        sLastError = "json_pool_alloc_failed";
        return false;
    }

    const DeserializationError tError = deserializeJson(
        oProject, &sProjectJsonBacking[0], sProjectJsonBacking.length());
    String sError;

    if (tError || !validateProject(oProject.as<JsonVariantConst>(), sError))
    {
        sLastError = tError ? String(tError.c_str()) : sError;
        Serial.printf("[PROJECT] Boot parse error=%s bytes=%u used=%u/%u free=%lu\n",
                      sLastError.c_str(), static_cast<unsigned>(iFileBytes),
                      static_cast<unsigned>(oProject.memoryUsage()),
                      static_cast<unsigned>(oProject.capacity()),
                      static_cast<unsigned long>(ESP.getFreeHeap()));
        return false;
    }

    JsonArrayConst tPages = oProject["pages"].as<JsonArrayConst>();
    sCurrentPage = String(tPages[0]["id"] | "");
    bProjectReady = true;
    sLastError = "";
    return true;
}

bool DashboardRuntime::saveProject(const String& sJson)
{
    if (sJson.isEmpty() || sJson.length() > iMaxConfigBytes)
    {
        sLastError = "invalid_size";
        return false;
    }

    // ESP32-3248S035 has little internal RAM. Do NOT allocate a second 32 KB
    // DynamicJsonDocument for validation: reuse the existing project document.
    // If anything fails, reload the previous project from LittleFS.
    const bool bHadProject = LittleFS.exists(sProjectFile);
    auto fRestoreProject = [this, bHadProject](const String& sError)
    {
        bool bRestored = false;

        if (bHadProject)
        {
            bRestored = loadProject();
        }

        if (bRestored)
        {
            renderPage(sCurrentPage);
        }
        else
        {
            bProjectReady = false;
            showDefault();
        }

        sLastError = sError;
        return false;
    };

    // One persistent mutable backing buffer avoids duplication of every key
    // and string in the ArduinoJson pool. Never release it while widgets exist.
    sProjectJsonBacking = sJson;
    oProject.clear();
    if (oProject.capacity() == 0)
    {
        return fRestoreProject("json_pool_alloc_failed");
    }
    if (sProjectJsonBacking.length() != sJson.length())
    {
        return fRestoreProject("json_body_copy_failed");
    }
    const DeserializationError tError = deserializeJson(
        oProject, &sProjectJsonBacking[0], sProjectJsonBacking.length());
    String sValidationError;

    if (tError || !validateProject(oProject.as<JsonVariantConst>(), sValidationError))
    {
        String sError = tError ? String(tError.c_str()) : sValidationError;
        Serial.printf("[PROJECT] Parse failed: %s (json=%u, used=%u/%u, heap=%lu)\n",
                      sError.c_str(), static_cast<unsigned>(sJson.length()),
                      static_cast<unsigned>(oProject.memoryUsage()),
                      static_cast<unsigned>(oProject.capacity()),
                      static_cast<unsigned long>(ESP.getFreeHeap()));
        if (tError == DeserializationError::NoMemory)
        {
            sError = "NoMemory:json_pool_used=" + String(oProject.memoryUsage()) +
                     ":capacity=" + String(oProject.capacity()) +
                     ":heap=" + String(ESP.getFreeHeap());
        }
        return fRestoreProject(sError);
    }

    // Preserve the previous active file until the new JSON is fully written.
    File oFile = LittleFS.open(sProjectTemporary, "w");

    if (!oFile)
    {
        return fRestoreProject("storage_open_failed");
    }

    const size_t iWritten = oFile.print(sJson);
    oFile.flush();
    oFile.close();

    if (iWritten != sJson.length())
    {
        LittleFS.remove(sProjectTemporary);
        return fRestoreProject("storage_write_failed");
    }

    File oVerify = LittleFS.open(sProjectTemporary, "r");

    if (!oVerify || oVerify.size() != sJson.length())
    {
        if (oVerify)
        {
            oVerify.close();
        }

        LittleFS.remove(sProjectTemporary);
        return fRestoreProject("storage_verify_failed");
    }

    oVerify.close();
    LittleFS.remove(sProjectBackup);

    if (bHadProject && !LittleFS.rename(sProjectFile, sProjectBackup))
    {
        LittleFS.remove(sProjectTemporary);
        return fRestoreProject("backup_failed");
    }

    if (!LittleFS.rename(sProjectTemporary, sProjectFile))
    {
        if (bHadProject)
        {
            LittleFS.rename(sProjectBackup, sProjectFile);
        }

        LittleFS.remove(sProjectTemporary);
        return fRestoreProject("activation_failed");
    }

    // The document is ALREADY parsed and validated. Never deserialize it a
    // second time during this HTTP request: that would repeat the RAM spike.
    JsonArrayConst tPages = oProject["pages"].as<JsonArrayConst>();
    sCurrentPage = String(tPages[0]["id"] | "");
    bProjectReady = true;
    sLastError = "";
    bInfoVisible = false;
    renderPage(sCurrentPage);
    Serial.printf("[PROJECT] Activated: bytes=%u, used=%u/%u, heap=%lu, page=%s\n",
                  static_cast<unsigned>(sJson.length()),
                  static_cast<unsigned>(oProject.memoryUsage()),
                  static_cast<unsigned>(oProject.capacity()),
                  static_cast<unsigned long>(ESP.getFreeHeap()),
                  sCurrentPage.c_str());
    return true;
}

void DashboardRuntime::showTouch(uint16_t iX, uint16_t iY, bool bPressed)
{
    (void)iX;
    (void)iY;

    if (!bPressed)
    {
        ulTouchDown = 0;
        bTouchHoldTriggered = false;
        return;
    }

    if (ulTouchDown == 0)
    {
        ulTouchDown = millis();
        return;
    }

    if (!bSetupMode && !bTouchHoldTriggered && millis() - ulTouchDown >= 6000U)
    {
        bTouchHoldTriggered = true;
        bPortalRequested = true;
    }
}
