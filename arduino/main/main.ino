/*
 * Projet Final IoT -- Module 5
 * Mastere Expert Dev Logiciel / Mobile / IoT -- Ynov Campus 2025-2026
 *
 * Description : [TODO -- décrire votre cas d'usage en 1-2 lignes]
 *               Ex: Station météo connectée -- mesure température + humidité
 *                   Transmission LoRaWAN vers TTN -> Node-RED -> InfluxDB -> Grafana
 *
 * Matériel    : ESP32 + RAK3172 + [votre capteur]
 * Auteur(s)   : [TODO -- Prenom Nom]
 * Date        : [TODO -- date de creation]
 *
 * Chaîne complète :
 *   Capteur -> ESP32 -> RAK3172 -> LoRaWAN -> TTN -> MQTT -> Node-RED -> InfluxDB -> Grafana
 *
 * IMPORTANT : renommer config.h.example en config.h avant de compiler.
 *             Ne pas committer config.h (contient les clés OTAA).
 */

// ---------------------------------------------------------------------------
// Includes
// ---------------------------------------------------------------------------
#include "config.h"

// TODO : inclure la bibliothèque de votre capteur
// Ex: #include <DHT.h>         (DHT22)
//     #include <Adafruit_BMP280.h>  (BMP280)
//     #include <Wire.h>        (I2C générique)

// ---------------------------------------------------------------------------
// Variables globales persistantes (RTC Memory -- survivent au Deep Sleep)
// ---------------------------------------------------------------------------
RTC_DATA_ATTR int boot_count = 0;  // Compteur de cycles (diagnostic)

// ---------------------------------------------------------------------------
// Prototypes de fonctions
// ---------------------------------------------------------------------------
float lire_capteur();
String encoder_payload(float valeur);
bool envoyer_lora(String payload_hex);
void attente_join(int timeout_s);

// ---------------------------------------------------------------------------
// setup() -- Execute a chaque reveil (Deep Sleep = reset complet)
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    boot_count++;
    Serial.printf("[INFO] Démarrage -- cycle #%d\n", boot_count);

    // -- Initialisation capteur --
    // TODO : initialiser votre capteur ici
    // Ex: dht.begin();
    //     if (!bmp.begin(0x76)) { Serial.println("[ERREUR] BMP280 non trouve"); }

    // -- Lecture de la valeur --
    float valeur = lire_capteur();
    Serial.printf("[CAPTEUR] Valeur mesurée : %.2f\n", valeur);

    // -- Encodage du payload --
    String payload_hex = encoder_payload(valeur);
    Serial.printf("[LORA] Payload hex : %s\n", payload_hex.c_str());

    // -- Envoi LoRaWAN --
    bool succes = envoyer_lora(payload_hex);
    if (!succes) {
        Serial.println("[ERREUR] Échec envoi LoRa -- passage en Deep Sleep quand même");
    }

    // -- Deep Sleep jusqu'au prochain cycle --
    Serial.printf("[SLEEP] Mise en veille pour %d secondes\n", SLEEP_DURATION_S);
    Serial.flush();
    esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_DURATION_S * 1000000ULL);
    esp_deep_sleep_start();
}

// ---------------------------------------------------------------------------
// loop() -- Jamais atteint (Deep Sleep provoque un réinitialisation)
// ---------------------------------------------------------------------------
void loop() {}

// ===========================================================================
// FONCTIONS METIER -- A implementer
// ===========================================================================

/**
 * lire_capteur()
 * Lit la valeur brute du capteur et la retourne en grandeur physique.
 * Retourne -999.0 en cas d'erreur de lecture.
 *
 * TODO : implémenter selon votre capteur
 */
float lire_capteur() {
    // Exemple DHT22 :
    //   float t = dht.readTemperature();
    //   if (isnan(t)) return -999.0;
    //   return t;

    // Exemple BMP280 :
    //   return bmp.readTemperature();

    // TODO : remplacer par votre implementation
    return -999.0;  // Placeholder
}

/**
 * encoder_payload()
 * Encode une valeur flottante en payload hexadécimal LoRaWAN 2 octets.
 * Convention : valeur * 10 -> int16 -> 2 octets big-endian (ex: 23.5 -> 235 -> "00EB")
 *
 * Adapter le facteur de multiplication selon votre capteur :
 *   - Température (0.1 degrés) : valeur * 10
 *   - Humidité (0.1 %) : valeur * 10
 *   - Pression (entier hPa) : (int)valeur
 */
String encoder_payload(float valeur) {
    if (valeur <= -999.0) return "FFFF";  // Code d'erreur

    // TODO : adapter le facteur selon votre grandeur physique
    int16_t raw = (int16_t)(valeur * 10.0);  // Ex: 23.5 °C -> 235

    char buf[5];
    snprintf(buf, sizeof(buf), "%04X", (uint16_t)raw);
    return String(buf);
}

/**
 * envoyer_lora()
 * Configure le module RAK3172, effectue le Join OTAA si nécessaire,
 * et envoie le payload sur le port 2.
 * Retourne true si l'envoi est confirmé, false sinon.
 *
 * TODO : vérifier les pins UART selon votre câblage
 *        Par défaut : RAK3172 sur UART2 (GPIO 16=RX, 17=TX)
 */
bool envoyer_lora(String payload_hex) {
    // -- Configuration UART vers RAK3172 --
    // TODO : adapter les pins selon votre cablage
    Serial2.begin(115200, SERIAL_8N1, 16, 17);  // RX=GPIO16, TX=GPIO17
    delay(500);

    // -- Configuration LoRaWAN (OTAA) --
    Serial2.println("AT+NWM=1");   delay(500);  // LoRaWAN
    Serial2.println("AT+NJM=1");   delay(500);  // OTAA
    Serial2.println("AT+BAND=4");  delay(500);  // EU868

    // TODO : configurer DevEUI, AppEUI, AppKey depuis config.h
    // Serial2.println("AT+DEVEUI=" + String(DEVEUI)); delay(500);
    // Serial2.println("AT+APPEUI=" + String(APPEUI)); delay(500);
    // Serial2.println("AT+APPKEY=" + String(APPKEY)); delay(500);

    // -- Join OTAA --
    Serial2.println("AT+JOIN=1:0:10:8");  // Tentatives : 8, délai : 10s
    attente_join(60);  // Attendre jusqu'à 60 secondes

    // -- Envoi du payload --
    String cmd = "AT+SEND=2:" + payload_hex;
    Serial2.println(cmd);
    delay(2000);

    // TODO : lire la réponse et retourner true si "+EVT:SEND_CONFIRMED" ou "+EVT:TX_DONE"
    return true;  // Placeholder
}

/**
 * attente_join()
 * Boucle d'attente du Join OTAA avec délai d'attente.
 * Affiche les messages du RAK3172 sur le port Serial de debug.
 */
void attente_join(int timeout_s) {
    unsigned long debut = millis();
    while (millis() - debut < (unsigned long)timeout_s * 1000) {
        if (Serial2.available()) {
            String ligne = Serial2.readStringUntil('\n');
            ligne.trim();
            Serial.println("[RAK] " + ligne);
            if (ligne.indexOf("JOINED") >= 0) {
                Serial.println("[LORA] Join OTAA réussi");
                return;
            }
        }
        delay(100);
    }
    Serial.println("[LORA] Timeout join OTAA");
}
