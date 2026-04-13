# Grafana -- Dashboard du projet

## Contenu de ce dossier

- `dashboard.json` : export JSON du dashboard Grafana (à compléter par l'étudiant)

## Exporter votre dashboard depuis Grafana

1. Ouvrir Grafana : `http://<ip_raspberry>:3000`
2. Ouvrir votre dashboard
3. Cliquer sur l'icône **Paramètres** (engrenage en haut à droite)
4. Section **JSON Model** dans le menu gauche
5. Cliquer **Copy to clipboard** ou sélectionner tout et copier
6. Coller dans `grafana/dashboard.json`
7. Committer : `git add grafana/dashboard.json && git commit -m "feat: export dashboard Grafana"`

## Importer le dashboard dans Grafana

1. Menu latéral gauche -> **Dashboards** -> **Import**
2. Cliquer **Upload JSON file** et sélectionner `dashboard.json`
3. Ou coller le contenu JSON dans le champ **Import via panel JSON**
4. Cliquer **Load** puis **Import**

> Note : vérifier que le nom de la source de données dans le JSON correspond
> à celle configurée dans votre Grafana (par défaut : `InfluxDB`).

## Panneaux attendus pour la soutenance (minimum 3)

| Panneau | Type | Description |
|---|---|---|
| Évolution temporelle | Time series | Historique sur 24h ou 7 jours |
| Valeur actuelle | Stat ou Gauge | Dernière mesure avec seuil de couleur |
| Compteur d'alertes | Stat | Nombre de dépassements de seuil |

## Exemple de requete Flux (Time series)

```flux
from(bucket: "iot_data")
  |> range(start: -24h)
  |> filter(fn: (r) => r._measurement == "capteur")
  |> filter(fn: (r) => r.device_id == "esp32-01")
  |> filter(fn: (r) => r._field == "température")
```

## Exemple de requete Flux (derniere valeur -- Stat/Gauge)

```flux
from(bucket: "iot_data")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "capteur")
  |> filter(fn: (r) => r.device_id == "esp32-01")
  |> filter(fn: (r) => r._field == "température")
  |> last()
```
