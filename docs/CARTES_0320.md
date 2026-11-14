# Matrice cartes — DashboardESP V0.3.20

| Carte | Resolution | LCD/tactile | Profil | Etat |
|---|---:|---|---|---|
| ESP32-3248S035C | 320x480 | ST7796 TFT_eSPI / GT911 | dashboardesp_035c_portrait | fonctionne chez l'utilisateur sur version precedente ; changement V0.3.20 non teste |
| ESP32-3248S035R | 320x480 | ST7796 / XPT2046 | dashboardesp_035r_* | ancien non valide |
| Waveshare ESP32-S3 LCD 7 | 800x480 | ST7262 RGB / GT911 | dashboardesp_waveshare_s3_7_landscape | fonctionne chez utilisateur sur version precedente ; changement V0.3.20 non teste |
| Waveshare ESP32-S3 LCD 4.3 | 800x480 | ST7262 RGB / GT911 | dashboardesp_waveshare_s3_43_landscape_EXPERIMENTAL | nouveau, timings non valides |
| ESP32-2432S028R | 240x320 | ILI9341 / XPT2046 | dashboardesp_2432s028r_portrait/landscape | nouveau experimental, calibrage tactile a faire |
| ESP32-2432S024R | 240x320 | ILI9341 / XPT2046 (variante) | dashboardesp_2432s024r_portrait_EXPERIMENTAL | nouveau, GPIO/revision a confirmer |
| ESP32-1732S019 | 170x320 | ST7789 / GT911 (C) ou sans touch (N) | platforms/esp32-1732s019 | materiel reference, pilote NON porte |
| ESP32-P4-WIFI6-Touch-LCD-7B | 1024x600 | EK79007 MIPI-DSI / GT911, C6 Wi-Fi | platforms/esp32-p4-wifi6-touch-lcd-7b ESP-IDF | demo ecran/tactile seulement ; JSON/HTTP NON porte |

ATTENTION : les noms ESP32-2432S028 / 024 recouvrent plusieurs variantes (R/C, ILI9341/ST7789, XPT2046/CST820). Ne pas televerser sur la seule base du nom commercial : comparer PCB, controleur et pinout. Profil experimental ne veut pas dire pilote prouve.

Le P4 utilise ESP-IDF et n'est pas un profil Arduino/PlatformIO de ce projet. Revisions materielle P4 possibles.
