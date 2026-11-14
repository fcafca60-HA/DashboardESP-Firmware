# Audit proprietes DashboardESP Studio / Firmware V0.3.20

**Important : compatibilite JSON ≠ rendu visuel 100% identique.** Les moteurs HTML/CSS et LVGL sont differents. Cette version reduit les ecarts de police mais ne constitue pas une validation pixel par pixel.

| Propriete JSON | Firmware | Commentaire |
|---|---|---|
| schema, pages, widget type, id | Oui | schema dashboardesp/1 ; limites pages/widgets et RAM |
| resolution.width/height | Oui | validation cible et mise a l echelle |
| x, y, w, h | Oui | transformation/echelle de canevas |
| background page/widget | Oui | les valeurs `rgba()` ne sont pas gerees comme une composition alpha CSS |
| color | Oui | #RRGGBB ; l'alpha CSS est simplifie |
| radius | Oui | mis a l echelle V0.3.20 |
| fontSize 8..48 | Oui / quantifie | familles Montserrat paires embarquees ; rendu different du navigateur |
| align left/center/right | Oui pour label primaire | certains widgets complexes imposent leur mise en page interne |
| text | Oui | accents convertis par le plugin avant envoi, selon configuration |
| command_id, show_unit, unit | Oui Valeur | les autres widgets ont des unites propres |
| weather_bindings.temperature/condition/humidity | Oui | icone meteo ASCII simplifiee |
| energy_bindings | Oui | solaire, maison, batterie, reseau / compteur |
| energy_mode, energy_max | Oui | mono/tri et niveau de jauge |
| thermostat_bindings | Oui | huit commandes info/action |
| symbol_key, symbol_on_key, symbol_on_color, symbol_off_color, symbol_on_above, symbol_hide_off, symbol_action_id | Oui | glyphes approximatifs LVGL |
| datetime_mode | Oui | horloge NTP locale |
| target_page_id | Oui | navigation |
| visual_style, accent_color, halo_effect | Partiel | couleur de bordure/ombre et pulse, ombres LVGL vs CSS differentes |
| dynamicThresholdsEnabled, dynamicThresholdBands | Oui sous conditions | couleurs et seuils selon valeur numerique |
| grid, snap, editor zoom | Editeur uniquement | ne doivent pas affecter l ESP |

Polices secondaires **meteo, energie, symbole et thermostat** utilisent des tailles dependantes de `fontSize` depuis V0.3.20. Les limitations de place sur petits ecrans peuvent masquer/rogner du texte. La commande `info` permet le diagnostic de la cible.

## Limites et verification

- Valider le rendu reeel sur 320x480 ESP32-3248S035C, puis sur Waveshare 800x480.
- Polices Montserrat et emojis/symboles de navigateur ne sont pas identiques. Une police Unicode/glyphes embarques supplementaires sera necessaire pour certains pictogrammes.
- Le firmware experimental P4 n'accepte pas encore le JSON / Wi-Fi / API Jeedom.
- Ne pas activer des actions tactiles sur des modeles experimentaux avant test de calibration.
