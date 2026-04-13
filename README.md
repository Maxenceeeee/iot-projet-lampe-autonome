# Projet Final IoT

## Module 5 -- Internet des Objets -- 2025-2026

> Soutenance : Mardi 05/05/2026 -- Format individuel ou groupe (~20-25 min)

---

## Description du projet

**Nom du projet :** _[A completer]_

**Auteur(s) :** _[Prenom Nom] ou [Prenom Nom] + [Prenom Nom]_

**Cas d'usage choisi :** _[Ex: Station meteo connectee, Detecteur de presence]_

**Capteur principal :** _[Ex: DHT22 (temperature/humidite), PIR, BMP280, LDR...]_

---

## Architecture de la chaine end-to-end

ESP32 (capteur) communique en LoRaWAN (OTAA, SF7, EU868) vers un RAK3172, puis TTN (The Things Network) transmet en MQTT (port 8883, TLS) vers un Raspberry Pi avec la stack MING : Mosquitto (broker, auth + TLS), Node-RED (decode + regles metier), InfluxDB (stockage serie temporelle), Grafana (dashboard visualisation).

> Voir aussi : docs/architecture.md pour le schema detaille.

---

## Livrables pour la soutenance

| Livrable | Fichier | Statut |
|---|---|---|
| Code Arduino complet | arduino/main/main.ino | TODO |
| Configuration secrets | arduino/config.h.example | TODO |
| Flow Node-RED exporte | nodered/flows.json | TODO |
| Dashboard Grafana exporte | grafana/dashboard.json | TODO |
| docker-compose.yml | docker/docker-compose.yml | TODO |
| Configuration Mosquitto | docker/mosquitto/mosquitto.conf | TODO |
| Schema d'architecture | docs/architecture.md | TODO |
| Fiche RGPD | docs/rgpd.md | TODO |

---

## Comment utiliser ce template

### 1. Utiliser ce template GitHub

Cliquer sur **Use this template** (bouton vert en haut a droite de la page GitHub). Nommer le repo iot-projet-votre-cas-dusage. Le repo peut etre public (soutenance) ou prive (ajouter intervenant comme collaborateur).

### 2. Completer les fichiers TODO

Remplacer tous les blocs marques TODO par votre implementation. Ne pas committer de secrets (cles API, tokens, mots de passe) -- utiliser config.h.example.

### 3. Exporter les fichiers depuis votre stack

**Node-RED :** Menu hamburger, Export, All flows, Clipboard, coller dans nodered/flows.json

**Grafana :** Dashboard, Parametres (engrenage), JSON Model, copier dans grafana/dashboard.json

### 4. Completer la fiche RGPD

Ouvrir docs/rgpd.md et remplir le tableau avec vos donnees reelles.

---

## Format soutenance

| Partie | Duree | Contenu attendu |
|---|---|---|
| Architecture | 5 min | Schema end-to-end, justification des choix techniques |
| Demo live | 5 min | ESP32 vers Grafana en direct devant le jury |
| Code commente | 5 min | Au moins 1 fichier Arduino + 1 flow Node-RED |
| Securite et RGPD | 5 min | Mesures appliquees (S8), lien Art. 32 RGPD |
| Questions | 5 min | Questions du jury |

> Conseil : Preparer la demo la veille. Verifier que la chaine complete fonctionne avant de commencer.

---

## Checklist avant la soutenance

- [ ] Code Arduino compile sans erreur
- [ ] ESP32 joint le reseau TTN (Live Data visible)
- [ ] Flow Node-RED receptionne et decode les donnees
- [ ] Donnees visibles dans InfluxDB (au moins 24h historique)
- [ ] Dashboard Grafana avec 3 panneaux minimum
- [ ] Authentification Mosquitto activee
- [ ] Dashboard exporte en JSON
- [ ] Fiche RGPD completee
- [ ] README.md mis a jour avec le nom du projet

---

## Ressources utiles

- Documentation TTN : https://www.thethingsnetwork.org/docs/
- Documentation InfluxDB Flux : https://docs.influxdata.com/flux/
- Documentation Grafana : https://grafana.com/docs/
- Documentation Node-RED : https://nodered.org/docs/
- Documentation ESP32 : https://docs.espressif.com/

---

_Template fourni par Baptiste Ochlafen - Internet des Objets M1 2025-2026_
