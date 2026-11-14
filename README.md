# DashboardESP Firmware — V0.3.19 (préparation GitHub)

**Source consultable — usage personnel non commercial uniquement.** Lire [LICENSE.txt](LICENSE.txt) avant utilisation. Pas de redistribution ou de versions dérivées sans autorisation écrite.

Projet de firmware d'écrans tactiles pour DashboardESP Studio / Jeedom. La base des cartes précédentes provient de V0.3.18 ; leur code d'affichage, tactile, réseau et JSON n'a pas été modifié dans cette livraison.

| Matériel | Environnement | Etat |
| --- | --- | --- |
| ESP32-3248S035C 320×480 | `dashboardesp_035c_portrait` | LCD/tactile validés par utilisateur ; envoi JSON encore en cours de validation |
| ESP32-3248S035R | `dashboardesp_035r_...` | profils présents, matériel non revalidé |
| Waveshare ESP32-S3 7 800×480 | `dashboardesp_waveshare_s3_7_landscape` | LCD/tactile/demo validés ; firmware réseau à confirmer |
| **ESP32-P4-WIFI6-Touch-LCD-7B 1024×600** | `platforms/esp32-p4-wifi6-touch-lcd-7b/` | **portage matériel initial, non testé, pas encore de synchronisation Jeedom** |

## Installation existante avec PlatformIO (VS Code)

Ouvrir le dossier racine contenant `platformio.ini`, sélectionner le profil correspondant à la carte, puis Build/Upload. Ne pas compiler le profil ESP32-S3 pour une carte P4.

## ESP32-P4 7B (nouveau, expérimental)

C'est un **projet ESP-IDF séparé**, et non un environnement PlatformIO qui remplacerait les autres. Voir [documentation P4](platforms/esp32-p4-wifi6-touch-lcd-7b/README.md). Il affiche une demo statique LVGL et un bouton de test tactile. Les fonctions réseau/JSON ne sont pas encore portées sur P4.

## Publication GitHub

Dépot : https://github.com/fcafca60-HA/DashboardESP-Firmware

Publier uniquement les sources, le dossier `platforms/`, le README, les docs et `LICENSE.txt`. Ne pas inclure `.pio`, les binaires locaux ou les secrets. Cette archive ne publie rien automatiquement sur GitHub.

## Historique du projet

# DashboardESP Firmware V0.3.18 — TFT_eSPI pour 3248S035C

L'afficheur 3248S035C utilise maintenant le meme pilote TFT_eSPI que le programme fonctionnel fourni (ST7796, SPI 27 MHz, GPIO 12/13/14/15/2, reset -1), et non plus LovyanGFX. Le moteur LVGL/JSON est conserve, l'ecran Waveshare est inchange. Le profil 3248S035R conserve LovyanGFX en attendant une reference materielle verifiee.

**Ordre de validation** : compiler et flasher `dashboardesp_035c_portrait_lcd_test` d'abord, puis `dashboardesp_035c_portrait` si l'image de test est stable. Le tactile GT911 sur SDA33/SCL32 est un essai distinct et doit etre verifie apres l'affichage : il n'etait pas specifie dans le projet TFT_eSPI fourni.

**Tests** : inspection statique et integrite ZIP; compilation PlatformIO et tests materiels non executes dans cet environnement.

# V0.3.18 - ESP32-3248S035 ST7796 bus fix

The user has validated a separate TFT_eSPI project on this physical board with
ST7796, MOSI=13, MISO=12, SCLK=14, CS=15, DC=2, RST=-1,
SPI_WRITE=27 MHz and SPI_READ=16 MHz. The existing DashboardESP pins already
matched, but its LovyanGFX backend used HSPI at 40 MHz. This release switches
only the 3.5-inch LovyanGFX backend to VSPI at 27 MHz.

**First run the isolated diagnostic** `dashboardesp_035c_portrait_lcd_test`
(no network, no LVGL, no JSON). It should display six stable color bands.
When this is confirmed, flash the regular `dashboardesp_035c_portrait` firmware.
The Waveshare RGB/GT911 driver is untouched.

Note: original known-good TFT_eSPI code uses its own initialization registers.
LovyanGFX's ST7796 initialization is not necessarily identical; if the
six-band test still corrupts, compare ST7796 init sequences rather than
assuming that these two SPI parameters alone are sufficient.

# DashboardESP Firmware V0.3.18 - Informations et cle API sur ecran

Le mode complet Waveshare affiche un bouton **INFO** en haut a droite des pages, de l ecran Wi-Fi et de l ecran d attente. Il ouvre une page avec IP, port 8090 et une cle API masquee par defaut. Appuyer sur AFFICHER LA CLE pour reveler le jeton (32 caracteres), MASQUER LA CLE pour le cacher et RETOUR pour revenir. CONFIG. WI-FI ouvre la configuration et demarre le point d acces. Le jeton reste dans Preferences et ne change pas au redemarrage. Le profil lcd_test/touch_test/demo reste inchange. Ne pas photographier ni partager la cle : l API utilise HTTP local non chiffre.

# DashboardESP Firmware V0.3.3 – Waveshare 7 pouces

## Matériel confirmé par essai utilisateur

- Waveshare ESP32-S3 Touch LCD 7 Rev 1.2 (800 × 480, orientation paysage)
- Affichage LCD fonctionnel et stable : pilote `Waveshare_ST7262_LVGL`, LVGL 8.3.11
- Tactile GT911 fonctionnel : appuis et curseur rouge validés sur la V0.3.0
- `ESP32_Display_Panel` v0.1.6, `ESP32_IO_Expander` v0.0.3 et pioarduino 53.03.11
- CH422G configuré via la bibliothèque. PSRAM OPI et Flash 8 MB comme dans ESPHome.

### 1. Nouveau test de reprise – SANS RESEAU (recommandé)

Depuis le dossier de ce projet :

```powershell
pio run -e dashboardesp_waveshare_s3_7_demo -t clean
pio run -e dashboardesp_waveshare_s3_7_demo
pio run -e dashboardesp_waveshare_s3_7_demo -t upload
pio device monitor -b 115200
```

- Affiche 2 pages LVGL provenant d'un JSON embarqué (`data/demo_waveshare_800x480.json`).
- Bouton **Page suivante** et **Retour accueil** : validation navigation tactile.
- Bouton **Lumière salon** : ON/OFF local uniquement, aucun ordre Jeedom réel.
- Valeurs 21.5 °C, 2450 W, 82 %; seconde page compare **avec et sans unité**.
- Le panneau et le tactile éprouvés ne sont pas modifiés. Wi-Fi/API absents de ce profil.
- Valeurs fictives : cette démo valide l'interface, pas la synchronisation Jeedom.

### 2. Firmware complet – RESEAU (étape suivante, non validée)

```powershell
pio run -e dashboardesp_waveshare_s3_7_landscape -t clean
pio run -e dashboardesp_waveshare_s3_7_landscape
```

Après compilation puis flash, assistant Wi-Fi à l'écran. Le firmware expose une API HTTP locale port **8090** et reçoit le JSON du plugin Jeedom. Le moteur 11 widgets est conservé (LVGL 8 sur 7 pouces, LVGL 9 sur 3,5 pouces).

**Important** : le mode complet reste à valider. Le passage `lib_ldf_mode=deep` corrige une cause documentée du `Network.h not found` provoqué par `deep+` avec Arduino 3.x, mais une compilation et les interactions HTTP réelles restent nécessaires.

Pour envoyer un projet, celui-ci doit être en 800 × 480 et respecter `schema=dashboardesp/1`. Les valeurs/commandes ont besoin d'une passerelle Jeedom (encore à développer dans le plugin). La mise en ligne automatique depuis Jeedom n'est pas encore implémentée.

### Profils conservés

- `dashboardesp_waveshare_s3_7_lcd_test` : bandes RGB, pilote validé.
- `dashboardesp_waveshare_s3_7_touch_test` : test GT911, pilote validé.
- `dashboardesp_waveshare_s3_7_demo` : nouvelle démo locale 2 pages.
- `dashboardesp_waveshare_s3_7_landscape` : firmware complet/API à confirmer.
- Quatre profils ESP32 3,5 pouces : conservés, non retestés.

Voir `docs/API.md` et `docs/REPRISE_V031.md`.


### V0.3.3: WiFi / Network.h build correction

For the Waveshare full build, `lib_ldf_mode = chain` replaces `chain+`.
Arduino ESP32 3.x / PlatformIO has a reported framework dependency detection
problem in the `+` LDF modes, where built-in `WiFi` cannot see `Network.h`.
The source already explicitly includes `<Network.h>`; no third-party
Network library should be installed. LCD, GT911, LVGL and JSON code are unchanged.

Build: `pio run -e dashboardesp_waveshare_s3_7_landscape -t clean`
then `pio run -e dashboardesp_waveshare_s3_7_landscape`.


## V0.3.18: compatibilite ESP32-3248S035

- La compilation des profils 3.5 pouces (Arduino ESP32 2.x) n'inclut plus `Network.h`, reserve au framework Arduino ESP32 3.x des profils Waveshare.
- Le rendu demo utilise Montserrat 24 pour les grandes valeurs afin de ne pas dependre de Montserrat 32, indisponible dans le profil 3.5 pouces observe.
- Aucun changement du pilote graphique ni des GPIO. Le firmware doit encore etre compile puis teste sur la carte physique.


## V0.3.18 : optimisation RAM ESP32-3248S035

- Profil 3,5 pouces LVGL 9 : reserve memoire statique reduite de 64 a 48 Kio.
- Buffer de dessin LVGL 3,5 pouces : 20 a 10 lignes (12 800 a 6 400 octets pour 320 pixels RGB565).
- RAM statique economisee : environ 22 784 octets, contre un depassement de 9 456 octets signale par le linker.
- Waveshare ESP32-S3 7 pouces : aucune modification aux buffers ni a LVGL 8.
- Environnement par defaut : dashboardesp_035c_portrait.

Le succes de la liaison et la memoire dynamique disponible lors du rendu sont a verifier sur la carte.


## V0.3.18 - GT911 tactile sur 3248S035C

- L'affichage ST7796 TFT_eSPI de V0.3.12 reste inchange.
- Initialisation GT911 avec pulse de RESET GPIO25 avant sondage I2C SDA33/SCL32.
- Recherche des adresses 0x5D et 0x14, identification GT911 par registre produit.
- En cas d'echec, trace I2C scan sur port serie.
- Profil test sans reseau, sans LVGL : `dashboardesp_035c_portrait_touch_test`.
- L'IRQ physique de certaines revisions 035C est reliee a GND : polling seulement.

Pour compiler et televerser :

`pio run -e dashboardesp_035c_portrait_touch_test -t upload --upload-port COM5`

Puis `pio device monitor -b 115200`. Si le pointeur suit le doigt, revenir au profil complet `dashboardesp_035c_portrait`.
