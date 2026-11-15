# DashboardESP Firmware V0.3.31 Beta

Mise à jour du firmware des tableaux de bord tactiles liés au plugin Jeedom DashboardESP Studio.

### Évolutions depuis V0.3.19 Beta

- Communication HTTP directe ESP → Jeedom pour les actions tactiles et synchronisation des états INFO.
- Économiseur d'écran à deux niveaux : horloge mobile puis écran noir.
- Améliorations des tuiles Météo et Énergie, des polices, du rendu lumineux et des symboles.
- Prise en charge des fonds d'écran par page : image transférée séparément du JSON et chargée avec protection mémoire.
- Ajout du composant Liste / Mode, **encore expérimental et non fiable pour tous les modes Jeedom**.
- Gestion mémoire améliorée : limite JSON jusqu'à 48 Kio pour Waveshare ESP32-S3 avec PSRAM ; 24 Kio maintenus sur ESP32 classiques.
- Profils expérimentaux pour plusieurs écrans ESP32 ; dossier ESP-IDF P4 toujours incomplet pour Jeedom.
- Corrections de compilation et de dépendances PlatformIO.

### Limites connues

- **Les composants Liste / Mode ne fonctionnent pas encore correctement dans toutes les configurations.** Ne pas les utiliser pour des actions critiques.
- Certaines cartes sont expérimentales et n'ont pas été validées sur matériel.
- Le rendu de fond d'écran est volontairement réduit en résolution pour limiter la RAM.
- La compilation PlatformIO et les validations fonctionnelles restent à refaire pour les nouveaux profils.

**Licence :** usage personnel non commercial, voir `LICENSE.txt`.

**Publication :** marquer cette release comme **Pre-release / Beta**, tag `v0.3.31-beta` sur la branche `main`. Ne pas remplacer l'ancienne release `v0.3.19-beta`.
