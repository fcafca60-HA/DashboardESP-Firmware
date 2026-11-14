/*
 * DashboardESP P4 / Waveshare LCD-7B - experimental LVGL/display/touch demo.
 * Copyright (c) 2026 DashboardESP author. See ../../LICENSE.txt.
 * This is not yet the full DashboardESP network/JSON runtime.
 * BSP handles MIPI DSI EK79007 + GT911 + display locking.
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"

static const char *sTag = "DashboardESP-P4";
static lv_obj_t *pStatus = NULL;
static uint32_t iTouchCount = 0;

static void OnTouch(lv_event_t *pEvent)
{
    if (lv_event_get_code(pEvent) != LV_EVENT_CLICKED) return;
    ++iTouchCount;
    if (pStatus != NULL)
    {
        lv_label_set_text_fmt(pStatus, "Tactile GT911 : %lu appui(s)", (unsigned long)iTouchCount);
    }
}

static lv_obj_t *CreateCard(lv_obj_t *pParent, int32_t iX, int32_t iY,
                           int32_t iWidth, int32_t iHeight, uint32_t iBackground)
{
    lv_obj_t *pCard = lv_obj_create(pParent);
    lv_obj_set_pos(pCard, iX, iY);
    lv_obj_set_size(pCard, iWidth, iHeight);
    lv_obj_set_style_radius(pCard, 18, 0);
    lv_obj_set_style_bg_color(pCard, lv_color_hex(iBackground), 0);
    lv_obj_set_style_border_color(pCard, lv_color_hex(0x19BCE9), 0);
    lv_obj_set_style_border_width(pCard, 2, 0);
    lv_obj_set_style_pad_all(pCard, 15, 0);
    lv_obj_remove_flag(pCard, LV_OBJ_FLAG_SCROLLABLE);
    return pCard;
}

static lv_obj_t *CreateLabel(lv_obj_t *pParent, const char *sText,
                             int32_t iX, int32_t iY, uint32_t iColor)
{
    lv_obj_t *pLabel = lv_label_create(pParent);
    lv_label_set_text(pLabel, sText);
    lv_obj_set_pos(pLabel, iX, iY);
    lv_obj_set_style_text_color(pLabel, lv_color_hex(iColor), 0);
    return pLabel;
}

static void BuildDemoUI(void)
{
    lv_obj_t *pScreen = lv_screen_active();
    lv_obj_set_style_bg_color(pScreen, lv_color_hex(0x081827), 0);
    lv_obj_set_style_bg_opa(pScreen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(pScreen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *pHeader = CreateCard(pScreen, 24, 24, 330, 96, 0x15385A);
    CreateLabel(pHeader, "DashboardESP", 18, 12, 0xE7F8FF);
    CreateLabel(pHeader, "ESP32-P4 WIFI6 LCD-7B", 18, 47, 0x63DDFF);

    lv_obj_t *pWeather = CreateCard(pScreen, 380, 24, 620, 145, 0x654516);
    CreateLabel(pWeather, "METEO  -  1024 x 600", 18, 16, 0xFFEAAA);
    CreateLabel(pWeather, "Demo sans donnees Jeedom", 18, 65, 0xFFFFFF);

    lv_obj_t *pThermo = CreateCard(pScreen, 24, 150, 520, 370, 0x063B38);
    CreateLabel(pThermo, "THERMOSTAT - DEMONSTRATION", 15, 12, 0xC9FFE7);
    lv_obj_t *pRoom = CreateCard(pThermo, 15, 65, 227, 105, 0x11515A);
    CreateLabel(pRoom, "Ambiante : 20.8 C", 12, 20, 0xB8FFCB);
    lv_obj_t *pTarget = CreateCard(pThermo, 253, 65, 227, 105, 0x11515A);
    CreateLabel(pTarget, "Consigne : 21 C", 12, 20, 0xB8FFCB);
    CreateLabel(pThermo, "Mode : ARRET  (simulation)", 18, 205, 0xE6F8FF);

    lv_obj_t *pButton = lv_button_create(pThermo);
    lv_obj_set_pos(pButton, 16, 260);
    lv_obj_set_size(pButton, 460, 65);
    lv_obj_add_event_cb(pButton, OnTouch, LV_EVENT_CLICKED, NULL);
    lv_obj_t *pButtonLabel = lv_label_create(pButton);
    lv_label_set_text(pButtonLabel, "TESTER LE TACTILE");
    lv_obj_center(pButtonLabel);

    lv_obj_t *pInfo = CreateCard(pScreen, 570, 190, 430, 330, 0x163554);
    CreateLabel(pInfo, "ESP32-P4 + ESP32-C6 Wi-Fi 6", 14, 16, 0xE5F6FF);
    CreateLabel(pInfo, "Ecran : EK79007 MIPI-DSI", 14, 58, 0xC8E8FF);
    CreateLabel(pInfo, "Tactile : GT911", 14, 101, 0xC8E8FF);
    CreateLabel(pInfo, "Projet JSON / API : A PORTER", 14, 144, 0xFFCF91);
    pStatus = CreateLabel(pInfo, "Tactile GT911 : 0 appui(s)", 14, 207, 0x75F3AD);

    CreateLabel(pScreen, "DashboardESP 0.3.19 - profil P4 experimental - non valide sur materiel", 35, 553, 0xA2D0E8);
}

void app_main(void)
{
    ESP_LOGI(sTag, "Starting ESP32-P4 LCD-7B display/touch preview");
    bsp_display_cfg_t tConfig = {
        .lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation = ESP_LV_ADAPTER_ROTATE_0,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL,
        .touch_flags = {
            .swap_xy = 0,
            .mirror_x = 1,
            .mirror_y = 1,
        },
    };
    lv_display_t *pDisplay = bsp_display_start_with_config(&tConfig);
    ESP_ERROR_CHECK(pDisplay != NULL ? ESP_OK : ESP_FAIL);
    bsp_display_backlight_on();
    ESP_ERROR_CHECK(bsp_display_lock(-1) ? ESP_OK : ESP_ERR_TIMEOUT);
    BuildDemoUI();
    bsp_display_unlock();
    ESP_LOGW(sTag, "P4: Wi-Fi/API/JSON not integrated in this experimental profile");
}
