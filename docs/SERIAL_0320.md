# Commandes serie DashboardESP V0.3.20

Port USB serie **115200 bauds, 8N1**. Select "Carriage return" ou "Both NL & CR" dans le terminal; CR et LF sont acceptes.

```
info
wifi MonSSID MonMotDePasse
wifi "Reseau Maison" "mot de passe avec espaces"
help
```

`info` affiche la version, le modele, la resolution, l'etat du Wi-Fi, l'adresse IP, le port **8090** et la **cle API**. La cle est un secret ; ne pas publier les journaux.

`wifi <ssid> <pwd>` valide la longueur, sauvegarde dans Preferences (NVS) puis **redemarre** l'ESP. La configuration ne necessite pas d'envoyer de JSON et n'efface pas le projet LittleFS. SSID maxi 32 octets et mot de passe 8..63 octets (ou `""` pour un reseau ouvert). Aucun mot de passe n'est reaffiche. Les champs contenant des espaces doivent etre entre guillemets; les guillemets inclus dans un champ ne sont pas pris en charge.

Sur les cartes ESP32-P4 experimentales, le runtime complet, l'API et ces commandes ne sont **pas encore portes**.
