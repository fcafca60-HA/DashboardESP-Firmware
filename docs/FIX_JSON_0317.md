# Publication JSON sans seconde allocation - V0.3.17

Cause : l’ESP32 classique réservait déjà 32 Ko pour `oProject` et tentait de créer un second `DynamicJsonDocument(32768)` durant `saveProject()`. Cette deuxième allocation pouvait échouer avec `NoMemory`.

Le firmware réutilise `oProject` pour valider les données transmises, écrit le fichier `project.tmp`, vérifie sa taille, effectue une sauvegarde `project.bak` et active directement le document en mémoire. En cas d'erreur avant activation, le projet précédent est rechargé depuis LittleFS. Le journal contient maintenant `used/capacity/heap`.

La version firmware est affichée sur l'écran INFO et le titre de configuration Wi-Fi. La page `/api/status` l'indique également.

Le rendu de chaque widget reste soumis à la RAM réellement disponible. Si un projet très chargé échoue encore, consulter le journal série `[PROJECT]` et l'API `last_error`.
