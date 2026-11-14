#include "DashboardDemo.h"

namespace
{
    // Embedded example: no filesystem upload, Wi-Fi or API needed for this hardware stage.
    // The same schema, command_id, show_unit and target_page_id keys as the Jeedom plugin.
    const char sDemoProject[] = R"json(
    {
      "schema":"dashboardesp/1", "resolution":{"width":800,"height":480},
      "pages":[
        {"id":"home","background":"#101827","widgets":[
          {"id":"title","type":"label","x":18,"y":15,"w":764,"h":55,"text":"DashboardESP - Waveshare 7 pouces","background":"#193555","color":"#ffffff","fontSize":24},
          {"id":"temp","type":"value","x":18,"y":90,"w":240,"h":140,"text":"21.5","command_id":1001,"show_unit":true,"unit":"°C","background":"#234361","color":"#ffffff","fontSize":32},
          {"id":"solar","type":"value","x":278,"y":90,"w":240,"h":140,"text":"2450","command_id":1002,"show_unit":true,"unit":"W","background":"#274d33","color":"#ffffff","fontSize":32},
          {"id":"soc","type":"value","x":538,"y":90,"w":244,"h":140,"text":"82","command_id":1003,"show_unit":true,"unit":"%","background":"#573c23","color":"#ffffff","fontSize":32},
          {"id":"light","type":"button","x":18,"y":260,"w":240,"h":96,"text":"Lumiere salon : OFF","command_id":2001,"background":"#315ca1","color":"#ffffff","fontSize":20},
          {"id":"nav","type":"page_link","x":278,"y":260,"w":240,"h":96,"text":"Page suivante >","target_page_id":"controls","background":"#386ca8","color":"#ffffff","fontSize":20},
          {"id":"hint","type":"label","x":18,"y":388,"w":764,"h":65,"text":"LCD + GT911 valides | TEST LOCAL : les commandes ne pilotent pas Jeedom","background":"#1d3045","color":"#d2e7ff","fontSize":16}
        ]},
        {"id":"controls","background":"#101827","widgets":[
          {"id":"title2","type":"label","x":18,"y":15,"w":764,"h":55,"text":"DashboardESP - Commandes tactiles","background":"#193555","color":"#ffffff","fontSize":24},
          {"id":"unitoff","type":"value","x":35,"y":120,"w":240,"h":120,"text":"21.5","command_id":1001,"show_unit":false,"unit":"°C","background":"#234361","color":"#ffffff","fontSize":28},
          {"id":"uniton","type":"value","x":300,"y":120,"w":240,"h":120,"text":"21.5","command_id":1001,"show_unit":true,"unit":"°C","background":"#234361","color":"#ffffff","fontSize":28},
          {"id":"switch","type":"button","x":555,"y":120,"w":200,"h":120,"text":"Appuyer ici","command_id":2002,"background":"#315ca1","color":"#ffffff","fontSize":20},
          {"id":"back","type":"page_link","x":240,"y":300,"w":320,"h":85,"text":"< Retour accueil","target_page_id":"home","background":"#386ca8","color":"#ffffff","fontSize":20},
          {"id":"footer","type":"label","x":20,"y":410,"w":760,"h":40,"text":"Sans unite         /         Avec unite        /        Action tactile","background":"#1d3045","color":"#d2e7ff","fontSize":16}
        ]}
      ]
    }
    )json";
}

DashboardDemo::DashboardDemo() : oProject(12288)
{
}

uint32_t DashboardDemo::parseColor(const String& sValue, uint32_t ulFallback)
{
    if (sValue.length() == 7 && sValue.charAt(0) == '#')
    {
        return static_cast<uint32_t>(strtoul(sValue.c_str() + 1, nullptr, 16));
    }
    return ulFallback;
}

void DashboardDemo::onWidgetClick(lv_event_t* pEvent)
{
    DemoWidget* pWidget = static_cast<DemoWidget*>(lv_event_get_user_data(pEvent));
    if (pWidget == nullptr || pWidget->pOwner == nullptr)
    {
        return;
    }
    DashboardDemo* pSelf = pWidget->pOwner;
    if (pWidget->sType == "page_link")
    {
        Serial.printf("[DEMO] PAGE -> %s\n", pWidget->sTarget.c_str());
        pSelf->sPendingPage = pWidget->sTarget;
    }
    else if (pWidget->sType == "button")
    {
        ++pSelf->iActionCount;
        pSelf->bLightOn = !pSelf->bLightOn;
        Serial.printf("[DEMO] TOUCH ACTION command_id=%d count=%d (local only)\n", pWidget->iCommandId, pSelf->iActionCount);
        if (pWidget->iCommandId == 2001)
        {
            lv_obj_t* pLabel = lv_obj_get_child(pWidget->pButton, 0);
            if (pLabel != nullptr)
            {
                lv_label_set_text(pLabel, pSelf->bLightOn ? "Lumiere salon : ON" : "Lumiere salon : OFF");
            }
        }
    }
}

void DashboardDemo::renderWidget(JsonObjectConst tWidget, lv_obj_t* pScreen)
{
    const String sType = String(tWidget["type"] | "label");
    const int iX = tWidget["x"] | 0;
    const int iY = tWidget["y"] | 0;
    const int iWidth = tWidget["w"] | 120;
    const int iHeight = tWidget["h"] | 50;
    const int iSize = tWidget["fontSize"] | 16;
    const uint32_t ulFill = parseColor(String(tWidget["background"] | "#193555"), 0x193555);
    const uint32_t ulText = parseColor(String(tWidget["color"] | "#ffffff"), 0xffffff);
    const bool bButton = (sType == "button" || sType == "page_link");

    lv_obj_t* pObject = bButton ? lv_btn_create(pScreen) : lv_obj_create(pScreen);
    lv_obj_set_pos(pObject, iX, iY);
    lv_obj_set_size(pObject, iWidth, iHeight);
    lv_obj_set_style_radius(pObject, 10, 0);
    lv_obj_set_style_bg_color(pObject, lv_color_hex(ulFill), 0);
    lv_obj_set_style_bg_opa(pObject, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(pObject, 0, 0);
    lv_obj_set_style_pad_all(pObject, 6, 0);
    lv_obj_clear_flag(pObject, LV_OBJ_FLAG_SCROLLABLE);

    String sText = String(tWidget["text"] | "");
    if (sType == "value" && (tWidget["show_unit"] | false))
    {
        const String sUnit = String(tWidget["unit"] | "");
        if (!sUnit.isEmpty())
        {
            sText += " " + sUnit;
        }
    }
    lv_obj_t* pLabel = lv_label_create(pObject);
    lv_label_set_text(pLabel, sText.c_str());
    lv_obj_set_width(pLabel, lv_pct(96));
    lv_obj_set_style_text_color(pLabel, lv_color_hex(ulText), 0);
    lv_obj_set_style_text_align(pLabel, LV_TEXT_ALIGN_CENTER, 0);
    const lv_font_t* pFont = iSize >= 24 ? &lv_font_montserrat_24 :
                             (iSize >= 20 ? &lv_font_montserrat_20 : &lv_font_montserrat_16);
    lv_obj_set_style_text_font(pLabel, pFont, 0);
    lv_obj_center(pLabel);

    if (bButton && iButtonCount < 16)
    {
        DemoWidget& tEntry = atButtons[iButtonCount++];
        tEntry.pOwner = this;
        tEntry.sType = sType;
        tEntry.sTarget = String(tWidget["target_page_id"] | "");
        tEntry.iCommandId = tWidget["command_id"] | 0;
        tEntry.pButton = pObject;
        lv_obj_add_event_cb(pObject, onWidgetClick, LV_EVENT_CLICKED, &tEntry);
    }
    else
    {
        lv_obj_clear_flag(pObject, LV_OBJ_FLAG_CLICKABLE);
    }
}

void DashboardDemo::renderPage(const String& sPageId)
{
    JsonArrayConst tPages = oProject["pages"].as<JsonArrayConst>();
    for (JsonObjectConst tPage : tPages)
    {
        if (String(tPage["id"] | "") != sPageId)
        {
            continue;
        }
        lv_obj_t* pScreen = lv_scr_act();
        lv_obj_clean(pScreen);
        iButtonCount = 0;
        for (int iIndex = 0; iIndex < 16; ++iIndex)
        {
            atButtons[iIndex] = DemoWidget();
        }
        lv_obj_set_style_pad_all(pScreen, 0, 0);
        lv_obj_set_style_bg_color(pScreen, lv_color_hex(0x101827), 0);
        lv_obj_clear_flag(pScreen, LV_OBJ_FLAG_SCROLLABLE);
        for (JsonObjectConst tWidget : tPage["widgets"].as<JsonArrayConst>())
        {
            renderWidget(tWidget, pScreen);
        }
        Serial.printf("[DEMO] page=%s rendered with %d touch controls\n", sPageId.c_str(), iButtonCount);
        return;
    }
    Serial.printf("[DEMO] page not found: %s\n", sPageId.c_str());
}

void DashboardDemo::begin()
{
    const DeserializationError tError = deserializeJson(oProject, sDemoProject);
    if (tError)
    {
        Serial.printf("[DEMO] JSON error: %s\n", tError.c_str());
        return;
    }
    renderPage("home");
    Serial.println("[DEMO] Offline JSON/touch validation. No WiFi or Jeedom requests.");
}

void DashboardDemo::loop()
{
    if (!sPendingPage.isEmpty())
    {
        const String sNext = sPendingPage;
        sPendingPage = "";
        renderPage(sNext);
    }
}
