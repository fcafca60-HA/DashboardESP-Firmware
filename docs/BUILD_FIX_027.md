# DashboardESP V0.2.7 - Alias CH422G (Waveshare 7 pouces)

Le journal du 12 novembre 2026 confirme que les anciennes erreurs `ESP_PANEL_*` sont resolues. Il ne reste que :

```text
ESP_IO_EXPANDER_I2C_CH422G_ADDRESS was not declared
ESP_IO_EXPANDER_I2C_CH422G_ADDRESS_000 suggested
```

L'environnement commun `[wave7]` de `platformio.ini` definit maintenant :

```ini
-D ESP_IO_EXPANDER_I2C_CH422G_ADDRESS=ESP_IO_EXPANDER_I2C_CH422G_ADDRESS_000
```

Cette definition s'applique aussi a la compilation de `Waveshare_ST7262_LVGL.cpp`, et relie l'ancien nom au symbole present dans ESP32_IO_Expander v0.0.3. Pas de modification manuelle des bibliotheques sous `.pio/libdeps`.

## Commandes

```powershell
pio run -e dashboardesp_waveshare_s3_7_lcd_test -t clean
pio run -e dashboardesp_waveshare_s3_7_lcd_test
```

Si un log ulterieur signale une autre incompatibilite, transmettre la premiere erreur du nouveau journal.

**Validation :** correction coherente avec le diagnostic du compilateur, inspection statique et integrity ZIP. Compilation PlatformIO complete non disponible ici.
