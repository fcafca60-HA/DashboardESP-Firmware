# V0.2.8 - Error Network.h in LCD-only build

The failed build compiled `DashboardRuntime.cpp`, which directly includes `<WiFi.h>`. In Arduino ESP32 3.1.1 `WiFi.h` depends on `Network.h`. This is not needed for LCD-only diagnostics.

Fix: `build_src_filter` for `dashboardesp_waveshare_s3_7_lcd_test` includes only `src/main.cpp` and `src/DashboardDisplay.cpp`. `main.cpp` avoids including and constructing `DashboardRuntime` when `DASH_LCD_DIAGNOSTIC` is defined.

To build: `pio run -e dashboardesp_waveshare_s3_7_lcd_test -t clean`, then `pio run -e dashboardesp_waveshare_s3_7_lcd_test`.

The complete Waveshare profile still includes WiFi by design; this patch does not assert it compiles.
