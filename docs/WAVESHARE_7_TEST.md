# Essai sur Waveshare ESP32-S3-Touch-LCD-7 Rev 1.2

## Identification

Profil cible : Waveshare **ESP32-S3-Touch-LCD-7**, écran ST7262 RGB565 800 x 480, tactile GT911 I2C sur SDA GPIO8 / SCL GPIO9, expandeur CH422G pour le rétroéclairage et les resets.

La référence commerciale et la révision doivent être vérifiées sur le PCB avant flashage. Ne pas utiliser ce profil sur un écran 7 pouces avec câblage différent.

## Plateforme

- Environnement : `dashboardesp_waveshare_s3_7_landscape`
- Variante portrait : `dashboardesp_waveshare_s3_7_portrait`
- `board = esp32-s3-devkitc-1`
- Flash 16 Mo, PSRAM 8 Mo OPI (`board_build.arduino.memory_type = qio_opi`)
- Partitions 16 Mo dédiées avec deux emplacements OTA réservés et LittleFS
- Dépendances : LovyanGFX 1.2.7, LVGL 9.2.2, ArduinoJson 6.21.5

## Premier essai

1. Extraire cette archive dans un **nouveau dossier**.
2. Ouvrir ce dossier dans VS Code + PlatformIO.
3. Compiler la cible :

   `pio run -e dashboardesp_waveshare_s3_7_landscape`

4. Si `SUCCESS`, brancher le port USB de programmation, fermer le moniteur série si nécessaire, puis téléverser :

   `pio run -e dashboardesp_waveshare_s3_7_landscape -t upload`

5. Ouvrir le moniteur :

   `pio device monitor -b 115200`

6. Au démarrage, vérifier le log `PSRAM`, puis l'initialisation du CH422G et du LCD. L'assistant Wi-Fi DashboardESP doit apparaître.
7. Toucher les champs SSID, mot de passe et nom ; contrôler que le clavier et les appuis sont cohérents.
8. Préparer ensuite un projet JSON via l'outil de transfert et tester la navigation et le thermostat.

## Notes de diagnostic

- **Écran noir** : vérifier sur la console `PSRAM`, puis `CH422G` ; le profil 3,5 pouces ne peut pas piloter cette dalle RGB.
- **Toucher décalé ou inactif** : GT911 peut utiliser `0x5D` ou `0x14` suivant la séquence de reset. Le profil initialise l'IRQ GPIO4 au niveau bas pour sélectionner `0x5D`.
- **Écran blanc / mauvaises couleurs** : contrôles des signaux RGB, du PCLK et du timing ; la configuration actuelle doit être validée physiquement.
- **Wi-Fi ne démarre pas** : inspecter le log série à 115200 bauds.
- **Projet 480x320** : le rendu est ajusté uniformément à 800x480, en 720x480 avec marges de 40 pixels. Les boutons internes complexes peuvent conserver des dimensions LVGL fixes, à vérifier.
- **Projet 800x480** : ce format est naturel pour la dalle, mais l'éditeur Jeedom actuel a encore une sélection 480x320 / 320x480. Ne pas modifier la résolution d'un ancien projet sans sauvegarde.
- **Branchement et alimentation** : utiliser le port et une alimentation USB adaptés à ce modèle ; ne pas câbler les broches écran à la main.

## Limites de cette version

Cette version est une **adaptation source non encore compilée sur l'environnement ESP32-S3 et non testée sur la carte réelle**. Les 11 widgets du moteur JSON précédent sont préservés. Le plugin Jeedom ne publie pas automatiquement les projets ; transfert de test possible avec `tools/dashboardesp_cli.py` (voir `docs/API.md`).
