#pragma once
// Runtime API compatibility: LVGL v8 (Waveshare) vs v9 (3.5-inch).
#include <lvgl.h>
#if defined(DASH_LVGL8)
#define lv_button_create lv_btn_create
#define lv_screen_active lv_scr_act
#define lv_obj_remove_flag lv_obj_clear_flag
#define lv_obj_delete lv_obj_del
#ifndef lv_pct
#define lv_pct(v) LV_PCT(v)
#endif
#endif
