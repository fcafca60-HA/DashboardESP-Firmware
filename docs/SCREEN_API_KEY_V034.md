# Afficher la cle API sur l ecran (V0.3.4)

Le profil **dashboardesp_waveshare_s3_7_landscape** comporte un bouton INFO en haut a droite des pages et des ecrans d attente/configuration Wi-Fi.

1. Appuyer sur INFO.
2. Lire l IP et le port 8090. Si l IP indique mode AP, connecter le Waveshare au reseau Wi-Fi de la maison avant de tester la connexion avec Jeedom.
3. Appuyer sur AFFICHER LA CLE pour reveler les 32 caracteres du jeton X-Dashboard-Token.
4. Reporter la cle et l IP dans le plugin DashboardESP Studio, puis utiliser Tester connexion ESP.
5. Appuyer sur MASQUER LA CLE puis RETOUR.

La cle est generee au premier demarrage et conservee dans Preferences (NVS). Elle ne change pas lors des redemarrages ni de la mise a jour classique du firmware. L API HTTP locale n est pas chiffree: ne pas exposer le port 8090 a Internet et ne pas publier la cle.

Le profil lcd_test / touch_test / demo ne contient pas le runtime Wi-Fi et ne presente donc pas cet ecran d informations.

Compilation : `pio run -e dashboardesp_waveshare_s3_7_landscape`

Ce projet a ete verifie statiquement, mais non compile/teste sur le materiel lors de la creation de cette archive.
