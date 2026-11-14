# Diagnostic brut GT911 V0.3.15 (ESP32-3248S035C)

Compiler et televerser `dashboardesp_035c_portrait_touch_test`.

Ce profil affiche STATUS, RAW X/Y et les huit octets lus a 0x8150, sans bornage.
Tester le centre et les quatre coins. Au toucher, STATUS est normalement 0x81 et les octets doivent varier.
Si RAW reste 65535 / BYTES FF FF..., la lecture I2C renvoie des donnees invalides ; ne pas essayer de corriger par rotation.
Si les valeurs brutes varient mais pas celles affichees dans le firmware normal, corriger l'orientation du tactile.
Envoyer trois lignes du moniteur serie : sans toucher, au centre et en glissant.
Le pilote ST7796 et les profils Waveshare n'ont pas ete modifies.


## Correction V0.3.15 : decodeur GT911

Les registres X_LOW/X_HIGH/Y_LOW/Y_HIGH debutent a `0x8150`.
Le TRACK_ID est a `0x814F` et n'est PAS inclus dans la lecture a `0x8150`.
L'ancien decodeur sautait donc a tort le premier octet.
Exemple issu du test video : `AE 00 54 01` -> X=174, Y=340.
Cette correction vaut pour le test RAW ET les boutons tactiles du firmware complet.
L'orientation doit encore etre confirmee par l'utilisateur sur les quatre coins.
