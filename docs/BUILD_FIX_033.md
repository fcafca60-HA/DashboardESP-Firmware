# V0.3.3 - WiFi Network.h correction

**Symptom:** `framework-arduinoespressif32/libraries/WiFi/src/WiFiGeneric.h:44:10: fatal error: Network.h: No such file or directory` while compiling `dashboardesp_waveshare_s3_7_landscape`.

**Root cause:** the Waveshare profile inherited `lib_ldf_mode = chain+`. This mode evaluates preprocessor conditionals, but an Arduino ESP32 3.x / PlatformIO library dependency regression can prevent WiFi from finding the built-in Network library.

**Fix:** Set `lib_ldf_mode = chain` in `[wave7]`. The runtime already explicitly includes `<Network.h>` before `<WiFi.h>`, so the plain chain mode can identify both libraries without adding a third-party package.

**Do not** download a random Network library or edit files under `.platformio/packages`.

```powershell
pio run -e dashboardesp_waveshare_s3_7_landscape -t clean
pio run -e dashboardesp_waveshare_s3_7_landscape
```

Compile not run in the authoring environment; final result must be confirmed on the user's PlatformIO setup.

Reference: https://github.com/espressif/arduino-esp32/issues/9782
