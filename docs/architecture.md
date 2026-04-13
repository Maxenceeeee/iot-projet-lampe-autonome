# Architecture du projet -- [TODO : nom du projet]

**Auteur(s) :** _[TODO -- Prenom Nom]_
**Cas d'usage :** _[TODO -- description]_
**Capteur :** _[TODO -- modèle + grandeur physique mesurée]_

---

## 1. Chaine end-to-end

```
+------------------+     LoRaWAN (OTAA)      +------------------+
|   ESP32          |------------------------>|   RAK3172        |
|   + [capteur]    |  SF7, EU868, duty 1%   |   (module LoRa)  |
|   Deep Sleep     |                         +------------------+
|   RTC Memory     |                                  |
+------------------+                                  | LoRaWAN
                                                       v
                                          +-------------------------+
                                          |   TTN (The Things       |
                                          |   Network)              |
                                          |   - Network Server      |
                                          |   - Application Server  |
                                          +-------------------------+
                                                       |
                                                       | MQTT (port 8883, TLS)
                                                       | Topic: v3/{app_id}/devices/...
                                                       v
                                         +-----------------------------+
                                         |   Raspberry Pi (Edge)      |
                                         |   [ Stack MING ]           |
                                         |                             |
                                         |  Mosquitto (broker MQTT)   |
                                         |      | auth + TLS          |
                                         |      v                     |
                                         |  Node-RED (traitement)     |
                                         |   - decode payload hex     |
                                         |   - regles metier          |
                                         |      |                     |
                                         |      v                     |
                                         |  InfluxDB (stockage)       |
                                         |   bucket: iot_data         |
                                         |      |                     |
                                         |      v                     |
                                         |  Grafana (visualisation)   |
                                         |   dashboard + alertes      |
                                         +-----------------------------+
```

---

## 2. Parametres LoRaWAN

| Parametre | Valeur | Justification |
|---|---|---|
| Mode | OTAA | Plus sécurisé qu'ABP (re-négociation des clés à chaque join) |
| Spreading Factor | SF7 | TODO : adapter selon la distance et la consommation |
| Bande | EU868 | Europe (8 canaux, 125 kHz BW) |
| Duty cycle | 1% | Réglementation ETSI EU868 |
| DevEUI | [dans config.h] | Identifiant unique du device |

---

## 3. Encodage du payload

**Convention :** valeur * [TODO facteur] => int16 => 2 octets big-endian hex

| Grandeur | Exemple | Calcul | Payload hex |
|---|---|---|---|
| Température (TODO) | 23.5 °C | 235 = 0x00EB | `00EB` |
| [TODO : autre grandeur] | [TODO] | [TODO] | [TODO] |

**Code de décodage (nœud Function Node-RED) :**

```javascript
// Décodage payload hex depuis TTN
var payload = msg.payload;

// Extraction du payload bytes depuis le message TTN
// msg.payload.uplink_message.frm_payload est en base64
var bytes = Buffer.from(payload.uplink_message.frm_payload, 'base64');

// Décodage big-endian int16 -> valeur réelle
// TODO : adapter le facteur selon votre grandeur
var raw = (bytes[0] << 8) | bytes[1];
var valeur = raw / 10.0;  // Ex: 235 -> 23.5

// Construction du message pour InfluxDB
msg.payload = {
    measurement: "capteur",  // TODO : adapter
    tags: {
        device_id: payload.end_device_ids.device_id,
        cas_usage: "TODO"  // TODO : adapter
    },
    fields: {
        température: valeur  // TODO : renommer selon votre grandeur
    }
};

return msg;
```

---

## 4. Securite (apres S8 -- RGPD Art. 32)

| Composant | Mesure appliquee | Justification RGPD |
|---|---|---|
| Mosquitto | Authentification (password_file) | Controle d'acces au broker |
| Mosquitto | TLS port 8883 | Chiffrement en transit |
| InfluxDB | Token API renouvele | Controle d'acces a la BDD |
| Grafana | Mot de passe admin change | Controle d'acces au dashboard |
| Grafana | allow_sign_up=false | Restriction des acces |
| LoRaWAN | Chiffrement AES-128 (natif) | Chiffrement radio (built-in OTAA) |

---

## 5. Flux de données (DFD simplifié)

```
[Capteur physique]
    --> mesure brute (ex: 235 = 23.5 °C)
    --> encodée en hex (2 octets)
    --> transmis via LoRaWAN (chiffré AES-128)
    --> décodé par TTN
    --> publié sur MQTT (chiffré TLS)
    --> consommé par Node-RED
    --> décodé en JSON
    --> stocké dans InfluxDB (bucket iot_data)
    --> visualisé dans Grafana
```

---

_TODO : ajouter un schéma Excalidraw ou Draw.io de l'architecture si possible_
_Remplacer tous les TODO par vos valeurs réelles avant la soutenance_
