# API DashboardESP V0.2.0

Base : `http://IP_ESP:8090`

**En-tête obligatoire** pour toutes les routes `/api/*` : `X-Dashboard-Token: <token>` (token propre à l'ESP, affiché par le moniteur série). Les routes `/` et `/setup` servent uniquement à la configuration initiale/locale.

| Méthode | Route | Fonction |
|---|---|---|
| GET | `/api/status` | Matériel, version, IP, page, RSSI, heap |
| GET | `/api/config` | Retourne le JSON actuellement enregistré |
| POST | `/api/config` | Envoi du projet JSON `dashboardesp/1` (body brut JSON) |
| POST | `/api/values` | Mise à jour des Info Jeedom par ID |
| GET | `/api/actions` | Lit et vide les commandes Action en attente |
| POST | `/api/page` | Corps brut : identifiant de page à afficher |
| POST | `/api/reset-wifi` | Rouvre l'assistant et le point d'accès |

### Valeurs

```json
{
  "values": {
    "1001": 21.5,
    "1002": 19,
    "1003": "Nuageux",
    "1004": 57,
    "1011": 2500
  }
}
```

### Actions

Réponse d'exemple à `GET /api/actions` :

```json
{"actions": [2001, 2001, 2015]}
```

Les commandes sont envoyées sous forme d'IDs, **sans leur exécution effective**. Le code consommateur doit retrouver ces IDs dans les associations autorisées par le projet Jeedom.

`GET /api/actions` vide la file en une fois. En cas d'erreur réseau après lecture mais avant traitement, les événements peuvent être perdus (pas d'acquittement fiable dans cette version). Les appels non authentifiés retournent HTTP 401.

## Actions thermostat V0.2.1

Les commandes simples restent des entiers. Une action de consigne est un objet :

```json
{"actions":[2001,{"command_id":2300,"value":21.5}]}
```

L'integration Jeedom devra executer `2300` en action/slider avec `slider=21.5`, apres validation des droits et associations. Aucun ordre ne sera execute par l'ESP lui-meme.

## V0.2.3 — Unité de mesure (widget `value`)

Exemple : `{"type":"value","command_id":123,"show_unit":true,"unit":"°C"}`. Le firmware ajoute l'unité une seule fois à l'affichage, sans modifier la valeur brute utilisée pour les seuils.
