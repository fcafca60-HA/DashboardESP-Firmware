# DashboardESP Firmware V0.2.9 - ESP_Panel link fix

## Root cause
`ESP32_Display_Panel` v0.1.6 compiles its `ESP_Panel` class implementation only with `ESP_PANEL_USE_BOARD` defined; the latter is produced from `ESP_PANEL_USE_CUSTOM_BOARD` in `ESP_Panel_Board_Internal.h`. Missing custom board configuration led to `undefined reference to ESP_Panel::...` at link time.

## Fix
- Add `include/ESP_Panel_Board_Custom.h` with 800x480 ST7262 RGB pins, GT911 touch on shared I2C, CH422G controlled by Waveshare library.
- Define `ESP_PANEL_USE_CUSTOM_BOARD=1` in `[wave7]` build flags.
- Keep original ESP32 3.5-inch targets unchanged.

## Test
```
pio run -e dashboardesp_waveshare_s3_7_lcd_test -t clean
pio run -e dashboardesp_waveshare_s3_7_lcd_test
```

This package passes static file and archive checks but has not been compiled with the actual pioarduino toolchain nor flashed onto the hardware. If the linker error recurs, run `pio run -e dashboardesp_waveshare_s3_7_lcd_test -v` and check that the ESP_Panel.cpp compiler invocation includes `-I include` and the custom header.
