# ESP32-P4-WIFI6-Touch-LCD-7B — profil expérimental

**Statut : démonstration matérielle non compilée et non testée sur carte**. Ce dossier est indépendant des profils PlatformIO du projet principal.

## Caractéristiques documentées

- ESP32-P4NRW32 + coprocesseur ESP32-C6 Wi-Fi 6 par SDIO (ESP-Hosted)
- écran 7 pouces **1024 × 600** MIPI-DSI, contrôleur **EK79007**, tactile **GT911**
- Flash 32 Mo, PSRAM 32 Mo
- BSP `waveshare/esp32_p4_wifi6_touch_lcd_7b` **3.0.1**, LVGL **9.5.0**

Sources officielles :
- https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-7B
- https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-7B/tree/main/examples/esp-idf/08_lvgl_display_panel

## Compiler sur un PC avec ESP-IDF 5.5.5

Installer l'extension **Espressif IDF** de VS Code et ESP-IDF v5.5.5 (distincte de PlatformIO).

```shell
cd platforms/esp32-p4-wifi6-touch-lcd-7b
idf.py set-target esp32p4
idf.py build
idf.py -p COMx flash monitor
```

Vérifier la révision matérielle avant flash, en particulier le mode `postv3` du composant officiel. Aucun port COM n'est préconfiguré.

## Ce que fait le programme

- Démarre le BSP officiel (MIPI-DSI, rétroéclairage, GT911) et LVGL 9.
- Présente un écran DashboardESP 1024×600 avec cartes météo, thermostat, statut et bouton tactile.
- Incrémente un compteur d'appuis sur le bouton de test.

## Ce qui n'est **pas encore porté**

- Le moteur d'interprétation du JSON DashboardESP (`schema=dashboardesp/1`).
- La sauvegarde des projets, le serveur HTTP/8090, la clé API et la publication depuis Jeedom.
- Le Wi-Fi via le coprocesseur ESP32-C6/ESP-Hosted.

Ne pas sélectionner un ancien environnement `dashboardesp_waveshare_s3_7_landscape` pour cette carte : le P4 est RISC-V/MIPI-DSI et **non compatible** avec le firmware ESP32-S3/RGB ST7262.

Un profil matériel préparé n'est **pas** une compatibilité du firmware complet annoncée comme opérationnelle.
