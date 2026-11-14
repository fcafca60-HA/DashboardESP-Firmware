# Synchronisation DashboardESP Studio V0.2.8

1. Flasher le profil `dashboardesp_waveshare_s3_7_landscape`.
2. Ouvrir le moniteur série 115200 : relever `Local API token` (32 hex).
3. Si Wi-Fi vierge, connecter au point d’accès `DashboardESP-xxxx` avec les 12 premiers caractères de la clé et ouvrir `http://192.168.4.1:8090/` ou adresse indiquée au moniteur série, puis saisir SSID/mot de passe Wi-Fi 2,4 GHz.
4. Dans Studio, renseigner IP, port 8090 et clé API de l'ESP, sauvegarder l’équipement.
5. Adapter le projet à 800x480, enregistrer, Tester connexion, Envoyer vers ESP. Vérifier réponse et affichage.
6. Actualiser mesures/actions ou attendre le cron Jeedom (environ 1 minute).

La communication repose sur HTTP non chiffré. Utiliser exclusivement un réseau privé de confiance ; ne jamais exposer 8090 sur Internet. Le token est affiché au démarrage série et enregistré dans un fichier JSON Jeedom (administrateurs).

**Important** : le profil complet est expérimental et la compilation PlatformIO n’a pas été vérifiée dans cet environnement. Les profils `lcd_test`, `touch_test` et `demo` restent disponibles et validés précédemment sur le matériel utilisateur.

Note : la récupération des valeurs/actions est orchestrée côté Jeedom ; les périodes dépendent du cron plugin. Si ce cron n’est pas configuré/activé, utiliser « Actualiser mesures / actions » dans Studio.
