## 0.3.19 - Préparation de publication (14/11/2026)

- Ajout d'un **profil séparé ESP32-P4-WIFI6-Touch-LCD-7B** ESP-IDF 5.5.5, BSP Waveshare 3.0.1 et LVGL 9.5.0.
- Demo 1024×600 MIPI-DSI/GT911 de validation tactile ; aucun faux environnement PlatformIO pour le P4.
- Mise a jour de la documentation et de la licence personnelle non-commerciale.
- Firmware existant 3248S035 et ESP32-S3 conserve sans changements de pilote ou de protocole.
- Compilation P4 et tests materiels **non effectues** ; JSON/HTTP/ESP-Hosted non portes sur P4.

## V0.3.18 - Publication JSON / ESP32 035C

- Correction `activation_load_failed:NoMemory` : la validation utilise le document JSON actif au lieu d'allouer un second document de 32 Ko.
- Activation directe du document valide après écriture et renommage LittleFS, sans second chargement.
- Réouverture du projet précédent en cas d'échec ; journalisation RAM et usage du document.
- Version du firmware visible dans la page INFO et dans le titre Wi-Fi.
- Pilotes TFT_eSPI/GT911 et Waveshare inchangés.

## V0.3.18
- Publication JSON : mémoire temporaire libérée avant relecture, contrôle du fichier temporaire, retour arrière robuste et diagnostic HTTP 422 explicite.

## V0.3.15 - GT911 decodage des coordonnees
- Correctif du decodage X/Y a partir de 0x8150 (offset +1 supprime).
- Exemple video : AE 00 54 01 donne X=174 et Y=340.
- Aucun changement de pilote ST7796/TFT_eSPI, ni de GPIO.
- Test reel sur la carte encore necessaire.

# V0.3.15
- Diagnostic brut GT911: donnees du registre 0x8150, status, octets, coordonnees non bornees.
- Ne plus ramener silencieusement les valeurs invalides 0xFFFF aux limites 319/479.
- Pilote TFT_eSPI ST7796 et configuration LCD conserves.
- Pas de changement du plugin Jeedom.

## V0.3.13 - GT911 035C tactile
- Pulse RESET 25, sondage I2C 33/32 (0x5D/0x14), scan diagnostic, profil touch_test TFT_eSPI autonome.
- LCD et backend Waveshare inchanges.
- Verification materielle et compilation PlatformIO complete requises.

## V0.3.12
- Pour les profils ESP32-3248S035C uniquement : remplacement reel de LovyanGFX par TFT_eSPI 2.5.43, reprenant les GPIO et les frequences du projet fonctionnel communique.
- Transfert LVGL RGB565 par `pushImage` TFT_eSPI et buffer partiel conserve.
- I2C GT911 autonome sur les broches documentees dans le firmware existant (non encore valide sur le materiel).
- Aucun changement du backend Waveshare ou du 3248S035R.

## V0.3.11 - ST7796 SPI 3.5 inch display alignment

- Match proven TFT_eSPI configuration: VSPI host, SPI write 27MHz, read 16MHz.
- Preserve existing 320x480 native dimensions and GPIO mapping.
- Add standalone 3.5 inch LCD color-band diagnostic for capacitive portrait/landscape.
- Leave 7-inch Waveshare RGB driver unchanged.
- No PlatformIO full build or physical board testing performed here.

## V0.3.10 - Optimisation memoire des ESP32 3,5 pouces

- Corrige la reservation de DRAM excessive du profil ESP32 classique.
- LVGL 9 : memoire interne 48 Kio au lieu de 64 Kio.
- Buffer RGB565 : 10 lignes au lieu de 20, soit 6 400 octets economises.
- Profil par defaut 035c portrait.
- Profil Waveshare 7 pouces inchange.
- Verification de compilation et de fonctionnement physique encore necessaire.

## V0.3.9 - Compatibilite ESP32-3248S035
- Include `Network.h` uniquement sur Waveshare / Arduino ESP32 3.x.
- Demo : police 24 max au lieu de Montserrat 32.
- Profils et rendu Waveshare preserves.

## V0.3.8 - Correction de compilation (2026-11-13)

- Correction de la parenthese manquante dans `DashboardRuntime::updateWidget()` pour le widget meteo.
- Aucune modification du pilote Waveshare ou du schema JSON.
- Validation de syntaxe locale et archive ZIP uniquement; compilation PlatformIO non executee.

## V0.3.8 - Fix geometrie LVGL / controle rendu (2026-11-13)

- Forcer le recalcul du layout LVGL apres configuration des dimensions.
- Sous-composants meteo/thermostat positionnes depuis des dimensions physiques deterministes, au lieu de coordonnees LVGL non encore rafraichies.
- Ajout de logs `[LAYOUT]` et test de geometrie multi-tailles.
- API et pilote materiel inchanges.

## V0.3.6 — UI Premium / Waveshare
- Style Premium commun aux widgets via visual_style et halo_effect.
- Meteo avec labels LVGL separes (temperature, condition, humidite, icone ASCII de secours).
- Halo LVGL doux ou pulsant; pilote RGB/GT911 et endpoints HTTP conserves.
- Thermostat : pavés mieux contrastés; pilotage tactile conserve.
- Aucun rendu reel 100% pixel-identique garanti: rendu materiel a verifier.

## V0.3.5 — Thermostat natif LVGL
- Deux cellules pour ambiante et consigne, titre et etat separes.
- Boutons +/- et Confort/Eco/OFF selon bindings; desactivation si commande absente.
- Geometrie adaptee aux dimensions reelles du widget, y compris le 190x130.
- Texte des modes normalise pour les glyphes LVGL non pris en charge.
- Conservation LCD/GT911, Wi-Fi, securite et API de la V0.3.4.

## V0.3.4
- Ajout d une page tactile INFORMATIONS (IP / port / cle API revelee sur action volontaire).
- Bouton INFO sur les pages du firmware complet et l ecran de configuration Wi-Fi.
- Conservation de la cle dans Preferences et des pilotes LCD/GT911 valides.

## V0.3.3 - PlatformIO WiFi dependency fix

- Waveshare profile: `lib_ldf_mode = chain` instead of `chain+`.
- Preserves all proven LCD/touch, LVGL and JSON functionality of V0.3.2.
- Fix is based on the known Arduino ESP32 3.x Network.h resolution issue.
- Firmware still needs real PlatformIO compilation and hardware verification.

# Changelog DashboardESP

## V0.3.2 – reprise fonctionnelle après validation écran et tactile
- Nouveau profil `dashboardesp_waveshare_s3_7_demo` sans réseau, widgets et pages LVGL 8 basés sur JSON.
- Mesures avec/sans unité selon `show_unit`, navigation entre pages et actions tactiles locales.
- Firmware complet maintenu avec test de résolution du problème Network.h (LDF `deep` et inclusion explicite).
- Profils de diagnostic LCD/GT911 V0.3.0 et pilotes éprouvés préservés.

## V0.3.0 - Test tactile GT911

- Nouveau profil `dashboardesp_waveshare_s3_7_touch_test`, isole du Wi-Fi et du runtime.
- Affichage X/Y, compteur d'appuis et point de suivi sous le doigt; trace serie 115200.
- Pilote RGB, configuration ESP_Panel, GT911, bibliotheques et LCD TEST V0.2.9 inchanges.
- Validation hardware tactile et build PlatformIO en attente.

# V0.2.9 - LCD test isolation

- LCD diagnostic excludes DashboardRuntime.cpp and its WiFi, HTTP, DNS, and JSON dependencies.
- In LCD diagnostic main.cpp does not instantiate DashboardRuntime.
- No changes to production runtime or plugin.
- Full runtime and its Arduino 3.1 Network.h behavior still need separate validation.

# V0.2.7

- Alias de compatibilite pour la constante I2C CH422G de ESP32_IO_Expander v0.0.3.
- Correctif applique aux profils Waveshare 7 pouces, sans modifier les profils 3,5 pouces.
- Aucun changement aux widgets ou au contrat JSON.

## V0.2.6
- Correctif bootloader QIO 80 MHz pour les profils Waveshare.
- Verrouillage ESP32_Display_Panel 0.1.6, ESP32_IO_Expander 0.0.3, LVGL 8.3.11 pour compatibilite avec Waveshare_ST7262_LVGL.
- Guide de purge de cache des anciennes bibliotheques.

## V0.2.4 (diagnostic Waveshare RGB)
- CH422G output mode configured before setting output state.
- Panel power/reset/GT911 reset/backlight initialization order corrected.
- Optional `DASH_LCD_DIAGNOSTIC` with direct LovyanGFX RGB test patterns.
- Reduced diagnostic pixel clock to 12 MHz.
- All previous DashboardESP widgets/JSON code retained, bypassed only in diagnostic mode.

## V0.2.3
- Widget Valeur : support JSON `show_unit` et `unit`, sans doublon, seuils calculés sur valeur brute.
- Anciens JSON compatibles : unité masquée par défaut.

# Historique des versions

## V0.2.3 — Waveshare ESP32-S3-Touch-LCD-7 Rev 1.2

- Nouveau profil RGB 800x480 ST7262, tactile GT911, contrôle CH422G.
- Deux environnements PlatformIO paysage / portrait, en plus des quatre profils 3,5 pouces existants.
- Prise en charge du framebuffer RGB en PSRAM OPI et du partitionnement flash 16 Mo.
- Contrôle de la présence de PSRAM au démarrage et diagnostics CH422G.
- Mise à l'échelle uniforme centrée des projets JSON 480x320 ou 320x480 sur la dalle 7 pouces.
- Code C++ formaté avec les accolades sur lignes séparées et notation hongroise.
- Guide de validation matérielle Waveshare ajouté.

**État : sources préparées, compilation et tests matériels à confirmer.**

## V0.2.1

- Widget Thermostat : températures, consignes et actions tactiles.

## V0.2.5
- Backend Waveshare issu du projet CantineMenu (bibliotheque `Waveshare_ST7262_LVGL`).
- LVGL 8.3.11 pour le 7 pouces avec couche de compatibilite runtime.
- Test LCD couleurs independant du Wi-Fi.
- LVGL 9 pour 3.5 pouces preserve.
