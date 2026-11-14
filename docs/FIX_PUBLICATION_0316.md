# V0.3.16 – correction publication JSON / HTTP 422

- Libération du document de validation ArduinoJson avant relecture du projet.
- Vérification des octets du fichier temporaire avant activation.
- Retour arrière protégé, conservation de l’erreur initiale.
- Traces `[PROJECT]` sur la liaison série.
- Le code TFT_eSPI ST7796 et GT911 de la V0.3.15 ne change pas.

Si une erreur `resolution_mismatch` survient, vérifier les dimensions du projet : 320×480 en mode portrait 035C.

Compilation et validation physique non effectuées dans cet environnement.
