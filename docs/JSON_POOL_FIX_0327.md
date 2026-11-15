# DashboardESP Firmware V0.3.27 - allocation du pool JSON

## Symptôme
`ESP HTTP 422: json_pool_alloc_failed` lors de **Envoyer vers ESP** sur ESP32-3248S035C.

## Correction
Le `DynamicJsonDocument` de configuration, précédemment alloué durant la construction statique du runtime, est maintenant alloué dans `DashboardRuntime::begin()` (avant les allocations Wi-Fi). Sur les petits ESP32, un repli sur 20 Kio puis 16 Kio est effectué si le bloc de 24 Kio ne peut pas être réservé.

Ligne série attendue : `[JSON] Project pool capacity=24576 free_heap=...` (ou 20480/16384). Une capacité nulle indique toujours un vrai échec de réservation ; le message d'erreur HTTP donne alors la RAM disponible.

## Limites
Cette correction traite **l'échec d'allocation initiale**. Elle ne garantit pas qu'un projet JSON dépassant la capacité de secours pourra être parsé ; dans ce cas ArduinoJson renvoie `NoMemory:json_pool_used=...:capacity=...:heap=...` et le projet précédent reste conservé. Les profils Waveshare sont conservés, sans changement de pilotes.

## Vérification recommandée
1. Compiler et flasher le profil exact, par exemple `dashboardesp_035c_portrait`.
2. Ouvrir le moniteur série à 115200 bauds et vérifier `[JSON] Project pool capacity=...`.
3. Cliquer sur Envoyer vers ESP et examiner la réponse ; en cas de nouvelle erreur, relever la ligne JSON et `NoMemory`.
4. Vérifier que le projet déjà présent n'a pas été perdu et que les touches fonctionnent.

Tests réalisés : contrôles statiques et intégrité du ZIP. **Compilation ESP matérielle non exécutée dans cet environnement.**
