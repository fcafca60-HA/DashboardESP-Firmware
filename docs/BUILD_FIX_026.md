# DashboardESP V0.2.6 - correctif de compilation Waveshare 7 pouces

## Diagnostic du log du 12/11/2026

1. Le chemin bootloader_qio_40m.elf indique que la frequence Flash 40 MHz est encore heritee et qu'aucun fichier de ce nom n'est livre par la plateforme. Le profil Waveshare fixe `board_build.f_flash = 80000000L`.
2. Les symboles `ESP_PANEL_CHECK_FALSE_RET`, `ESP_PANEL_LCD_WIDTH`, `ESP_PanelBus_RGB` appartiennent a une ancienne generation de ESP32_Display_Panel. Le projet V0.2.5 utilisait la branche actuelle de cette bibliotheque, incompatible avec `Waveshare_ST7262_LVGL`.
3. Les versions verrouillees sont ESP32_Display_Panel v0.1.6, ESP32_IO_Expander v0.0.3, LVGL v8.3.11, conformement aux dependances declarees par Waveshare_ST7262_LVGL.

## Essai sous Windows / PlatformIO

Extraire cette archive dans un nouveau dossier. Depuis ce dossier :

```powershell
pio run -e dashboardesp_waveshare_s3_7_lcd_test -t clean
```

Pour supprimer les anciennes bibliotheques mises en cache seulement pour cet environnement :

```powershell
Remove-Item -Recurse -Force ".pio\libdeps\dashboardesp_waveshare_s3_7_lcd_test" -ErrorAction SilentlyContinue
```

Puis :

```powershell
pio run -e dashboardesp_waveshare_s3_7_lcd_test
```

Ne pas flasher tant que la compilation n'a pas reussi.

## Limite de validation

Le projet a subi les controles locaux de syntaxe de configuration, de structure et de JSON. Il n'a PAS ete compile avec les dependances Arduino/ESP-IDF sur cette machine. Les bibliotheques anciennes peuvent requerir une autre adaptation au framework Arduino 3 fourni par pioarduino ; dans ce cas, transmettre la premiere nouvelle erreur `error:`.

## Note

Cette version n'intervient pas sur le plugin Jeedom ni sur les fichiers d'equipements/projets sauvegardes dans Jeedom.
