# Architecture Firmware V0.2.0

## Pipeline

1. `DashboardDisplay`: écran ST7796 + tactile GT911 ou XPT2046 (profil compile-time).
2. `main.cpp`: intégration LCD/LVGL, flush RGB565, événements tactiles, tick.
3. `DashboardRuntime`: Wi-Fi STA, portail de configuration AP, Preferences (NVS), LittleFS, API HTTP locale.
4. `DashboardRuntime`: charge `dashboardesp/1`, valide la résolution, nombre de pages/widgets, construit les objets LVGL de la page active.
5. `POST /api/values` : mise à jour du cache des commandes Info par identifiant Jeedom, puis rafraîchissement des objets LVGL.
6. Appui sur un bouton : navigation locale via `target_page_id`, ou mise en file d'une commande Action dans `GET /api/actions`.

Le JSON exporté par le plugin contient **les identifiants des commandes**, pas les valeurs. Les valeurs devront donc parvenir à l'ESP via un pont Jeedom (non inclus dans V0.2.4 du plugin), sinon `--` est affiché.

## Widgets pris en charge

| Type | Mise en œuvre V0.2.0 |
|---|---|
| label | texte statique |
| value | valeur liée à `command_id`, plages `dynamicThresholdBands` |
| button | action mise en file (`command_id`) |
| rectangle | fond coloré, étiquette facultative |
| page_link | ouverture page `target_page_id` |
| datetime | NTP, date/heure/date et heure, fuseau Europe/Paris |
| weather | `weather_bindings.temperature/condition/humidity`, seuils sur température |
| energy_summary | `energy_bindings.solar/home/battery/grid`, signes préservés |
| energy_meter | `energy_mode` mono/tri, `energy_max`, jauge et valeurs |
| symbol | symbole/état/couleur/visibilité selon `command_id`, action `symbol_action_id` |

## Limites connues

- Prototype non compilé avec PlatformIO localement ; matériel non disponible.
- Pas de preview graphique à l'identique des polices CSS, pas de prise en charge des images arbitraires SVG/PNG.
- Pas de push automatique depuis le plugin V0.2.4, ni exécution automatique des actions en Jeedom : nécessite mise à jour distincte du plugin.
- API HTTP sans chiffrement : réseau local seulement.
- Stockage config max 24 KiB et projet en RAM, limites dimensionnées pour les ESP32 sans PSRAM ; vérifier heap sur carte réelle.
- Les premières valeurs d'exemple de `demo_project.json` utilisent des identifiants Jeedom fictifs et ne correspondent pas à votre installation.
- Le portail ne peut pas choisir automatiquement un SSID sur une liste ; SSID saisi manuellement.
- Adapter les calibres et coordonnées tactiles selon la révision R/C et les résultats de tests matériels.
