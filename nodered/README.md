# Node-RED -- Flows du projet

## Contenu de ce dossier

- `flows.json` : export de l'ensemble des flows Node-RED (à compléter par l'étudiant)

## Exporter vos flows depuis Node-RED

1. Ouvrir Node-RED dans le navigateur : `http://<ip_raspberry>:1880`
2. Menu hamburger (en haut à droite) -> **Export**
3. Sélectionner **All flows**
4. Cliquer sur **Download** ou **Copy to clipboard**
5. Si clipboard : coller le contenu dans `flows.json`
6. Committer le fichier : `git add nodered/flows.json && git commit -m "feat: export flows Node-RED"`

## Importer des flows dans Node-RED

1. Menu hamburger -> **Import**
2. Sélectionner le fichier `flows.json`
3. Cliquer **Import**
4. Déployer : bouton rouge **Deploy**

## Flow minimal attendu pour la soutenance

```
[TTN MQTT in] --> [JSON parse] --> [Function: decode payload]
                                       |
                              [Switch: seuil d'alerte]
                             /                        \
                  [InfluxDB out]               [Debug / Alerte]
```

Nœuds utilisés :
- `mqtt in` : connexion TTN (port 8883, TLS, credentials TTN)
- `json` : parse le message MQTT
- `function` : décode le payload hex (ex: `00EB` -> 23.5)
- `switch` : règle métier (si valeur > seuil)
- `influxdb out` : écriture dans le bucket `iot_data`

## Structure du payload decode (sortie du noeud function)

```json
{
  "measurement": "capteur",
  "tags": {
    "device_id": "esp32-01",
    "cas_usage": "meteo"
  },
  "fields": {
    "temperature": 23.5
  }
}
```

> Adapter les champs `measurement`, `tags` et `fields` à votre cas d'usage.
