# Portage driver Waveshare / CantineMenu

Reference : archive fournie `CantineMenuESP32_PlatformIO(1).zip`.

## Changements

| Domaine | Firmware V0.2.4 | Firmware V0.2.5 |
|---|---|---|
| Waveshare RGB | LovyanGFX Bus_RGB | Waveshare_ST7262_LVGL / ESP32_Display_Panel |
| Controle CH422G | Ecritures I2C manuelles | Bibliotheque fournie |
| GT911 | LovyanGFX | Bibliotheque fournie |
| PSRAM | qio_opi / Arduino 2 | qio_opi / pioarduino identique a Cantine |
| LVGL Waveshare | 9.2.2 | 8.3.11 |
| LVGL 3,5 pouces | 9.2.2 | 9.2.2 inchange |
| Format JSON / widget | dashboardesp/1 | dashboardesp/1 inchange |

Le mapping GPIO est defini dans les dependances Waveshare et non duplique ici.

## Concurrence LVGL

La bibliotheque lance son propre traitement LVGL ; `DashboardRuntime::loop` peut rendre depuis le contexte principal.
Un mutex `lvgl_port_lock` entoure les appels DashboardRuntime. Attention : le serveur HTTP est aussi traite sous mutex, et peut ralentir le rendu sous forte charge. C'est acceptable pour un premier test, mais la prochaine iteration doit isoler le rendu du traitement reseau.

## Limitations

- Portrait Waveshare non active : une vraie rotation du tampon RGB/tactile est necessaire. Le mode paysage est la priorite.
- La correspondance pixel-par-pixel LVGL8/9 des styles peut differer ; le format JSON est identique.
- Le systeme de synchronisation plugin -> ESP n'est toujours pas fini cote Jeedom.
- Verifier la compilation du code et le rendu reel sur le materiel avant utilisation continue.
