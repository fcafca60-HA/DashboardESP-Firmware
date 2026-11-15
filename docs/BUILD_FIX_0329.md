# DashboardESP Firmware V0.3.29 - Correctif GCC 13 ESP32-S3

Erreur corrigee: `no matching function for call to min(unsigned int, long unsigned int)` dans `DashboardRuntime.cpp`, au calcul de `ulTop` du degrade.

Les trois appels utilisent maintenant `min<uint32_t>` pour imposer un type commun. L avertissement GT911 n est pas la cause de l echec.

Test conseille: `pio run -e dashboardesp_waveshare_s3_7_landscape`

Pas de modification du plugin Jeedom.
