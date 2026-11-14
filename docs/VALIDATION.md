# Validation V0.2.0

## Validations effectuées dans l'environnement d'édition

- Fichiers JSON d'exemple parsés par Python.
- Inventaire statique des dix widgets, coordonnées, limites de taille et pages validé.
- Client HTTP Python sans dépendances externes vérifié au niveau syntaxe.

## Non encore vérifié

- Build PlatformIO des 4 environnements (environnement de génération hors ligne dépourvu de PlatformIO et des bibliothèques Espressif).
- Initialisation écran/tactile sur matériel réel.
- Conformité du LVGL 9.2.2 au code de configuration et compilation des includes.
- Mesures de RAM, temps de réponse du transfert JSON, stabilité sur la durée.
- Publication directe depuis le plugin Jeedom V0.2.4 (non implémentée dans le plugin).

## Checklist tests matériels

1. `pio run -e dashboardesp_035c_landscape` et les trois autres configurations.
2. Flasher le modèle physique correspondant et vérifier pixels, orientation, tactile.
3. Premier boot : assistant SSID, mot de passe, nom de l'ESP.
4. Configurer depuis le portail Wi-Fi `http://192.168.4.1:8090/`.
5. Visualiser `/api/status` avec jeton.
6. Envoyer `data/demo_project.json` (pour orientation **paysage uniquement**).
7. Envoyer `data/demo_values.json` et vérifier température, météo, compteur, énergie, symbole et seuils.
8. Tester navigation vers page 2 et retour accueil.
9. Toucher le bouton : `GET /api/actions` retourne l'identifiant de la commande.
10. Redémarrer et vérifier rechargement du projet.
11. Couper/rebrancher Wi-Fi, vérifier reconnexion et comportement des valeurs manquantes.
12. Tester plus de 24 widgets / plus de 12 pages / résolution différente : rejet avec erreur explicite.
13. Vérifier résistance des commandes hors réseau et remise à zéro de la file d'actions.
14. Test 48h sans fuite mémoire ni crash.

La compilation de l'ancienne V0.1.2 est **connue réussie** chez l'utilisateur ; cela ne valide pas cette V0.2.0.

## V0.3.17
Validation statique du flux JSON, des références, des archives. Compilation PlatformIO et essai matériel à effectuer.
