# Reprise du projet V0.3.2

## Base non modifiée
- `lcd_init()` de Waveshare, CH422G, RGB, PSRAM, GT911, configuration ESP_Panel personnalisée.
- V0.2.9 : LCD validé en conditions réelles.
- V0.3.0 : tactile GT911 validé en conditions réelles.

## Ajouts
- Démo locale JSON + LVGL 8, 2 pages, 3 mesures avec unités, exemple sans unité.
- Navigation tactile et action locale traçable dans le moniteur série.
- Réactivation du profil 7 pouces complet, compatible avec les routes HTTP existantes.
- `lib_ldf_mode=deep` à la place de `deep+` pour limiter le problème `Network.h` sous Arduino ESP32 3.x, avec inclusion explicite de `Network.h`.

## Limitations
- Test statique des sources et JSON fait; compilation PIO physique impossible dans cet environnement.
- Démo locale ne lit pas le système de fichiers, ne lance pas de Wi-Fi, ne transmet aucune action à Jeedom.
- Le profil complet reste expérimental. Il faut encore valider Wi-Fi, API, transfert de projet et passerelle Jeedom.
- Le format du projet et les versions du plugin Jeedom Studio V0.2.7 ne changent pas.
