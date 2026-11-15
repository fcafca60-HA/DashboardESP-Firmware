# DashboardESP Firmware — V0.3.31 Beta

Firmware pour tableaux de bord tactiles ESP32 conçus avec le plugin Jeedom **DashboardESP Studio**.

**Statut : Beta.** Le fonctionnement des widgets **Liste / Mode** reste imparfait et fera l'objet d'une version corrective ultérieure. Les nouveaux profils matériels signalés EXPERIMENTAL ne sont pas validés sur carte réelle.

## Fonctionnalités

- Écrans graphiques multipages configurés par JSON depuis Jeedom ; valeurs INFO et actions tactiles.
- Communication locale HTTP : états Jeedom vers ESP, actions tactiles vers Jeedom (sur configuration compatible).
- Widgets Valeur, Bouton, Météo, Énergie, Thermostat, Symbole et sélecteur Liste / Mode expérimental.
- Police configurable, arrondi 0/1/2 décimales, rendu avec relief/halo léger.
- Image de fond par page transférée séparément du JSON, avec réduction de résolution et contrôle des allocations.
- Économiseur à deux niveaux : horloge mobile puis écran noir.
- Commandes série à 115200 bauds : `info`, `wifi "SSID" "mot de passe"` et `help`.

## Matériel et état de validation

| Carte | Environnement / dossier | Statut |
| --- | --- | --- |
| ESP32-3248S035C | `dashboardesp_035c_portrait` / `dashboardesp_035c_landscape` | Fonctionnement déjà testé sur matériel ; fonctions nouvelles à revalider |
| ESP32-3248S035R | `dashboardesp_035r_portrait` / `dashboardesp_035r_landscape` | À valider |
| Waveshare ESP32-S3 Touch LCD 7 (800×480) | `dashboardesp_waveshare_s3_7_landscape` | Écran et tactile testés ; nouveau JSON 48 Kio à valider |
| Waveshare ESP32-S3 LCD 4.3 | `dashboardesp_waveshare_s3_43_landscape_EXPERIMENTAL` | Expérimental |
| ESP32-2432S028R | `dashboardesp_2432s028r_...` | Expérimental |
| ESP32-2432S024R | `dashboardesp_2432s024r_portrait_EXPERIMENTAL` | Expérimental |
| ESP32-1732S019C | `dashboardesp_1732s019c_portrait_EXPERIMENTAL` | Expérimental |
| ESP32-P4-WIFI6-Touch-LCD-7B | `platforms/esp32-p4-wifi6-touch-lcd-7b/` | Démonstration ESP-IDF séparée, sans moteur JSON Jeedom complet |

## Compilation

Ouvrir ce dépôt dans **Visual Studio Code** avec l'extension **PlatformIO** ; choisir un environnement du fichier `platformio.ini` correspondant *exactement* à la carte.

```powershell
pio run -e dashboardesp_waveshare_s3_7_landscape
pio run -e dashboardesp_waveshare_s3_7_landscape -t upload
```

Pour l'ESP32-3248S035C :

```powershell
pio run -e dashboardesp_035c_portrait
```

Le projet P4 utilise **ESP-IDF** dans son sous-dossier et non cet environnement PlatformIO.

## Mémoire et limites

- Waveshare S3 avec PSRAM : projet JSON envoyé jusqu'à **48 Kio** (réservation supérieure, sous réserve de mémoire disponible).
- ESP32 classiques : limite JSON **24 Kio**, avec dégradation contrôlée si l'allocation échoue.
- Image de fond **hors JSON**, optimisée par Jeedom et chargée seulement si la mémoire le permet.
- En cas de modification de la table des partitions, sauvegarder sa configuration puis renvoyer le projet.

## Documentation

- [Changelog technique](docs/CHANGELOG.md)
- [API](docs/API.md)
- [Connexion Jeedom](docs/SYNC_JEEDOM.md)
- [Fonds d'écran](docs/BACKGROUND_V0328.md)
- [Cartes et compatibilité](docs/CARTES_0320.md)
- [Licence](LICENSE.txt)

**Licence :** source consultable, usage personnel non commercial uniquement. Les versions modifiées et redistributions nécessitent les autorisations prévues dans [LICENSE.txt](LICENSE.txt). Les licences tierces s'appliquent séparément.

**Sécurité :** ne jamais publier les clés API, mots de passe Wi-Fi, secrets locaux, ni le contenu de `.pio/`.
