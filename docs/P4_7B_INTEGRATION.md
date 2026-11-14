# Etat du support ESP32-P4-WIFI6-Touch-LCD-7B

**V0.3.19 : support prepare et isole, demo ESP-IDF non testee.** Pas de compatibilite avec le JSON/serveur Jeedom annoncee a ce stade.

## Matériel

| Carte | LCD | Tactile | Wi-Fi | Framework |
| --- | --- | --- | --- | --- |
| ESP32-3248S035C | ST7796 320×480 SPI | GT911 | ESP32 classique | Arduino 2.x/TFT_eSPI |
| Waveshare S3 LCD 7 | ST7262 800×480 RGB | GT911 | ESP32-S3 | pioarduino/LVGL 8 |
| **ESP32-P4-WIFI6-Touch-LCD-7B** | **EK79007 1024×600 MIPI-DSI** | **GT911** | **ESP32-C6 / ESP-Hosted** | **ESP-IDF 5.5.5 / LVGL 9** |

Le P4 ne possede pas de Wi-Fi integre et ne peut reutiliser le pilote ST7262/RGB ni les GPIO du Waveshare S3.

## Integration prochaine

1. Valider sur vrai P4 : compilation, LCD, tactile, revision materielle.
2. Porter le lecteur du schema `dashboardesp/1` et les 11 widgets sous LVGL9.
3. Porter le stockage, les valeurs et actions et l'API HTTP 8090 avec protection du jeton.
4. Tester la gestion reseau ESP32-C6 avec ESP-Hosted et la synchronisation Jeedom.
5. Proposer la resolution 1024×600 dans Studio **uniquement quand la cible est exploitable**.

La prise en charge experimentale est isolee dans `platforms/esp32-p4-wifi6-touch-lcd-7b/` : aucun changement des profils anciens dans PlatformIO.

References : https://docs.waveshare.com/ESP32-P4-WIFI6-Touch-LCD-7B et https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-7B
