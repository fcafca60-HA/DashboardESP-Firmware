# V0.3.18 - ArduinoJson NoMemory on classic ESP32

- Retain the JSON payload in `sProjectJsonBacking` for the complete life of `oProject`.
- Parse mutable `char*` to avoid ArduinoJson duplicating all string keys and values.
- Reuse the same zero-copy approach at boot, when loading JSON from LittleFS.
- Reduce ArduinoJson pool from 32768 to 16384 bytes on 3.5-inch ESP32; Waveshare remains at 32768 bytes.
- On NoMemory, return JSON pool use/capacity and free heap to help distinguish an undersized pool from exhausted heap.
- LCD / TFT_eSPI / GT911 / WaveShare board profiles unchanged.

## Caution

DynamicJsonDocument references point into the persistent backing string. The backing string must never be modified without re-parsing the document and recreating widget references. `loadProject` and `saveProject` both perform a full parse, followed by `renderPage` on successful activation. Physical tests and full PlatformIO compilation are still required.
