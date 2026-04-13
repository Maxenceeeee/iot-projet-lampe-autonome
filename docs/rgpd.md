# Fiche RGPD -- Projet IoT

**Auteur(s) :** _[TODO -- Prenom Nom]_
**Cas d'usage :** _[TODO -- description]_
**Date :** _[TODO -- date de redaction]_

> Ce document est requis pour la soutenance (partie Sécurité & RGPD, 5 min).
> Il répond aux exigences de l'article 32 du RGPD et illustre la méthodologie
> de privacy-by-design appliquee au projet.

---

## 1. Donnees collectees (Art. 30 RGPD -- Registre des traitements)

| Donnée collectée | Type | Capteur | Fréquence | Durée de rétention | Qualifié de DCP ? |
|---|---|---|---|---|---|
| TODO (ex: Température) | Numérique (float) | TODO (ex: DHT22) | TODO (ex: 1/min) | 30 jours | Non -- donnée physique |
| TODO (ex: Humidité) | Numérique (float) | TODO (ex: DHT22) | TODO (ex: 1/min) | 30 jours | Non -- donnée physique |
| DevEUI du device | Identifiant | RAK3172 | À chaque join | 30 jours | **Potentiellement** -- identifiant unique |
| Métadonnées TTN (RSSI, SNR, gateway_id) | Métadonnée réseau | TTN | À chaque envoi | 30 jours | **Potentiellement** -- géolocalisation indirecte |

> **Donnée à caractère personnel (DCP) :** toute information permettant
> d'identifier directement ou indirectement une personne physique.
> Les données de capteurs environnementaux (température, humidité, CO2...) ne
> qualifient pas de DCP en elles-mêmes. Mais croisées avec une localisation ou
> un identifiant de device, elles peuvent le devenir.

### Analyse spécifique au cas d'usage

> TODO : décrire ici si les données collectées dans votre cas d'usage peuvent
> qualifier de DCP. Justifier en une phrase.
>
> Exemple (présence) : "Un capteur PIR détectant des mouvements dans un domicile
> privé permet d'inférer des habitudes de vie. Ces données qualifient de DCP
> au sens de l'art. 4(1) RGPD."
>
> Exemple (température extérieure) : "La température extérieure mesurée sur une
> terrasse ne permet pas d'identifier une personne. Ces données ne qualifient
> pas de DCP. Cependant, les métadonnées TTN (gateway_id, RSSI) permettant
> une géolocalisation indirecte sont traitées avec prudence."

---

## 2. Mesures techniques appliquees (Art. 32 RGPD)

| Mesure | Composant | Implémentation | Statut |
|---|---|---|---|
| Authentification broker MQTT | Mosquitto | password_file, allow_anonymous false | [x] S8 |
| Chiffrement en transit MQTT | Mosquitto | TLS port 8883, certificat auto-signé | [x] S8 |
| Chiffrement radio LoRaWAN | RAK3172/TTN | AES-128 natif, clés OTAA | [x] S3 |
| Contrôle d'accès base de données | InfluxDB | Token API, 1 token par service | [x] S8 |
| Contrôle d'accès tableau de bord | Grafana | Mot de passe admin, sign-up désactivé | [x] S8 |
| Minimisation des données | Payload hex | Payload 2 octets (valeur seule, sans métadonnées) | [x] S5 |
| Limitation de durée de rétention | InfluxDB | Rétention 30 jours (configurable) | [x] S5 |

### Mesures non appliquees et justification

| Mesure non appliquée | Raison | Impact résiduel |
|---|---|---|
| Certificat TLS signé par une CA reconnue | Contexte de formation, CA auto-signée | Risque d'attaque MITM sur le réseau local uniquement |
| Chiffrement au repos (InfluxDB) | Hors scope formation, ressources RPi limitées | Accès physique au RPi = accès données |
| Pseudonymisation device_id | Identification du device nécessaire pour le projet | Lien device <-> étudiant connu du seul intervenant |

---

## 3. Droits des personnes (Art. 15-22 RGPD)

> TODO : adapter selon votre cas d'usage.
>
> Si le projet collecte des données pouvant qualifier de DCP :

| Droit | Moyen d'exercice | Délai de réponse |
|---|---|---|
| Accès (art. 15) | Demande par email à [TODO : contact] | 30 jours |
| Rectification (art. 16) | Modification manuelle dans InfluxDB | 30 jours |
| Effacement (art. 17) | Suppression du bucket ou filtre temporel | 30 jours |
| Portabilité (art. 20) | Export CSV depuis InfluxDB Data Explorer | 30 jours |

> Si les données ne qualifient pas de DCP, mentionner : "Les données collectées
> ne qualifiant pas de DCP, les droits des personnes au titre du RGPD ne
> s'appliquent pas directement. Cependant, par souci de transparence, les
> données sont supprimées automatiquement après 30 jours."

---

## 4. Synthese -- Argumentaire soutenance (5 min)

> Utiliser ce bloc comme guide pour la partie RGPD de la soutenance.

**Point 1 : Identification des données sensibles**
_"Les données collectées par notre projet sont [TODO]. [Elles qualifient / ne qualifient pas] de DCP car [TODO justification]."_

**Point 2 : Mesures techniques appliquées (Art. 32)**
_"Nous avons appliqué les mesures suivantes en S8 : authentification Mosquitto, TLS port 8883, renouvellement du token InfluxDB, hardening Grafana. Ces mesures sont appropriées au risque identifié (accès non autorisé sur réseau local)."_

**Point 3 : Minimisation et rétention**
_"Le payload LoRaWAN est réduit à 2 octets (valeur seule). La rétention dans InfluxDB est limitée à 30 jours. Aucune donnée n'est transmise vers des serveurs tiers (Edge-only)."_

**Point 4 : Limites connues et améliorations possibles**
_"Notre CA est auto-signée (contexte formation). En production, nous utiliserions Let's Encrypt ou une PKI d'entreprise. Le chiffrement au repos n'est pas activé (contrainte RPi)."_

---

_Template fourni par Baptiste Ochlafen -- Internet des Objets M1 Ynov 2025-2026_
