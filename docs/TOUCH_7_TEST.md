# GT911 tactile Waveshare - procedure de validation

- Profil: `dashboardesp_waveshare_s3_7_touch_test`
- Carte: Waveshare ESP32-S3 Touch LCD 7, 800x480, orientation paysage.
- La V0.2.9 a valide l'ecran RGB, son cablage et les librairies pinnees.
- Cette V0.3.0 ajoute uniquement une couche UI LVGL 8 et un callback d'evenement tactile.
- Pour chaque pression, l'ecran montre X/Y, le nombre d'appuis et un point rouge.
- Moniteur serie a 115200: `[TOUCH] DOWN`, `[TOUCH] MOVE`, `[TOUCH] UP`.
- Tester coins, centre et deplacements continus; relever si les axes sont inverses/decales.
- Si aucun evenement: relever les lignes serie de `lcd_init()` et du GT911.
- Ni reseau Wi-Fi ni moteur JSON ne sont charges dans ce profil.
- La compilation complete PlatformIO et le fonctionnement du tactile restent a confirmer sur la cible.
