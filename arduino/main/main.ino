/*
 * Projet Final IoT -- Module 5
 * Mastere Expert Dev Logiciel / Mobile / IoT -- Ynov Campus 2025-2026
 *
 *  DESCRIPTION :
 *    Lecture de l'état lumineux (sombre / lumineux) via un module
 *    photorésistance LDR LM393 à sortie numérique (DO), et envoi
 *    LoRaWAN OTAA vers TTN toutes les 60 secondes.
 *    Une LED RGB indique l'état en temps réel :
 *      - Sombre   → couleur sombre (rouge/violet foncé)
 *      - Lumineux → couleur claire (blanc chaud)
 *    Transmission LoRaWAN vers TTN -> Node-RED -> InfluxDB -> Grafana
 *
 *    MODULE LM393 3 BROCHES — SORTIE NUMÉRIQUE UNIQUEMENT
 *    Ce module utilise un comparateur LM393 interne.
 *    La broche DO sort HIGH (lumineux) ou LOW (sombre) selon
 *    le seuil réglé par le potentiomètre (vis).
 *    Aucune résistance externe requise — pont diviseur intégré.
 *
 *  PAYLOAD : 1 octet
 *    0x00 = sombre   |   0x01 = lumineux
 *
 *  BRANCHEMENTS LDR :
 *    GPIO16 (RX2)  ←  TX du RAK3272S
 *    GPIO17 (TX2)  →  RX du RAK3272S
 *    GPIO32        ←  DO du module LDR LM393
 *    3.3V          →  VCC module LDR
 *    GND           →  GND module LDR
 *
 *  BRANCHEMENTS LED RGB (avec résistances 10kΩ) :
 *    GPIO18  →  [10kΩ]  →  R (rouge)
 *    GPIO19  →  [10kΩ]  →  G (vert)
 *    GPIO23  →  [10kΩ]  →  B (bleu)
 *    GND     →  Cathode commune (-)
 *
 * Matériel    : ESP32 + RAK3172 + Module de photorésistance LDR LDR LM393
 * Auteur(s)   : Guillet Evan + Frappier Maxence
 * Date        : 02-03-2026 
 *
 * Chaîne complète :
 *   Capteur -> ESP32 -> RAK3172 -> LoRaWAN -> TTN -> MQTT -> Node-RED -> InfluxDB -> Grafana
 *
 * IMPORTANT : renommer config.h.example en config.h avant de compiler.
 *             Ne pas committer config.h (contient les clés OTAA).
 */

// ========== CONFIGURATION BROCHES ==========
#define RXD2    16   // GPIO16 — RX2 ESP32  ←  TX RAK3272S
#define TXD2    17   // GPIO17 — TX2 ESP32  →  RX RAK3272S
#define LDR_PIN 32   // GPIO32 — Sortie DO du module LDR LM393

// ========== LED RGB (LEDC PWM) ==========
#define LED_R   18   // GPIO18 → résistance 10kΩ → broche R
#define LED_G   19   // GPIO19 → résistance 10kΩ → broche G
#define LED_B   23   // GPIO23 → résistance 10kΩ → broche B

// ========== CONFIGURATION TTN ==========
// ⚠️  Remplacez par vos valeurs depuis TTN Console
//     Applications → votre-app → Devices → votre device
const char* APPEUI = "AABBCCDD00112233";                 // 16 chiffres hex — Application EUI
const char* DEVEUI = "70B3D57ED005A1B2";                // 16 chiffres hex — Device EUI
const char* APPKEY = "1CE5BD8278ACA16A0441E62D8DA37DAF"; // 32 chiffres hex — App Key

// ========== PARAMÈTRES ==========
const unsigned long INTERVALLE_ENVOI = 60000; // Période d'envoi LoRaWAN (ms)
const int           NB_LECTURES_DO   = 5;     // Nombre de lectures pour vote majoritaire anti-rebond

// ---------------------------------------------------------------------------
// Includes
// ---------------------------------------------------------------------------
#include "config.h"

// ========== VARIABLES GLOBALES ==========
bool          loraJoined  = false; // Statut de la connexion OTAA
unsigned long dernierEnvoi = 0;    // Horodatage du dernier envoi (ms)


// ============================================================
//  LED RGB — Réglage couleur via LEDC PWM
// ============================================================
/*
 * Définit la couleur de la LED RGB via le périphérique LEDC de l'ESP32.
 * Chaque canal utilise un canal PWM indépendant (0, 1, 2).
 *
 * @param r  Intensité rouge  (0–255)
 * @param g  Intensité vert   (0–255)
 * @param b  Intensité bleu   (0–255)
 */
void setLED(uint8_t r, uint8_t g, uint8_t b) {
  ledcWrite(LED_R, r);
  ledcWrite(LED_G, g);
  ledcWrite(LED_B, b);
}


// ============================================================
//  SETUP
// ============================================================
void setup() {
  // Initialisation du port série de débogage (moniteur série)
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n============================================================");
  Serial.println(" ESP32-D + LDR LM393 + RAK3272S + LED RGB — LoRaWAN OTAA");
  Serial.println("============================================================\n");

  // Initialisation Serial2 pour la communication AT avec le RAK3272S
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  delay(300);

  // --- Configuration LED RGB (LEDC PWM) ---
  // Canal 0 → Rouge | Canal 1 → Vert | Canal 2 → Bleu
  // Fréquence 5000 Hz, résolution 8 bits (0–255)
  ledcAttach(LED_R, 5000, 8);
  ledcAttach(LED_G, 5000, 8);
  ledcAttach(LED_B, 5000, 8);
  setLED(0, 0, 0); // LED éteinte au démarrage
  Serial.println("[LED] RGB initialisée — GPIO18(R) GPIO19(G) GPIO23(B)");

  // Configuration de la broche DO du module LDR en entrée numérique
  // Aucun pull-up interne : le module LM393 pilote activement la broche
  pinMode(LDR_PIN, INPUT);
  Serial.println("[LDR] Module LM393 initialisé sur GPIO32 (sortie numérique DO)");

  // Test de lecture initiale du capteur + allumage LED selon état
  Serial.println("\n--- Test Capteur LDR ---");
  testLDR();

  // Envoi des commandes AT de configuration LoRaWAN au RAK3272S
  Serial.println("\n--- Configuration LoRaWAN ---");
  configurerLoRa();

  // Lancement du Join OTAA (attente jusqu'à 60 secondes)
  Serial.println("\n--- Join OTAA TTN ---");
  joinOTAA();

  Serial.println("\n--- Prêt ---");
  Serial.println("Envoi automatique toutes les 60 secondes.");
  Serial.println("Commandes disponibles : SEND | TEST | JOIN\n");
}


// ============================================================
//  LOOP
// ============================================================
void loop() {
  // --- Mise à jour LED RGB en temps réel ---
  // Lecture de l'état courant et mise à jour de la couleur
  int etatLDR = lireLDR();
  if (etatLDR == 1) {
    // LUMINEUX → blanc chaud (255, 255, 200)
    setLED(255, 255, 200);
  } else {
    // SOMBRE → rouge/violet très foncé (20, 0, 30)
    setLED(20, 0, 30);
  }

  // --- Envoi automatique périodique ---
  // Vérifie que le join OTAA est actif et que l'intervalle est écoulé
  if (loraJoined && (millis() - dernierEnvoi >= INTERVALLE_ENVOI)) {
    envoyerDonnees();
    dernierEnvoi = millis();
  }

  // --- Commandes manuelles depuis le moniteur série ---
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    if (cmd == "SEND") {
      Serial.println("\n[CMD] Envoi manuel");
      envoyerDonnees();
    }
    else if (cmd == "TEST") {
      Serial.println("\n[CMD] Test capteur LDR");
      testLDR();
    }
    else if (cmd == "JOIN") {
      Serial.println("\n[CMD] Join OTAA manuel");
      joinOTAA();
    }
    else if (cmd.startsWith("AT")) {
      // Transmission directe d'une commande AT brute au RAK3272S
      envoyerAT(cmd, 2000);
    }
  }

  // --- Écoute des événements asynchrones du RAK3272S ---
  // Le RAK envoie des événements (+EVT:JOINED, etc.) de façon autonome
  while (Serial2.available()) {
    String evt = Serial2.readStringUntil('\n');
    evt.trim();
    if (evt.length() > 0) {
      Serial.println("[RAK] " + evt);
      // Mise à jour du statut de connexion selon l'événement reçu
      if (evt.indexOf("+EVT:JOINED") >= 0)      loraJoined = true;
      if (evt.indexOf("+EVT:JOIN_FAILED") >= 0) loraJoined = false;
    }
  }
}


// ============================================================
//  LECTURE LDR — Vote majoritaire (anti-rebond numérique)
// ============================================================
/*
 * Effectue NB_LECTURES_DO lectures de la broche DO du LM393
 * et retourne l'état majoritaire pour éliminer les faux positifs.
 *
 * @return  1 si lumineux (DO majoritairement HIGH)
 *          0 si sombre   (DO majoritairement LOW)
 */
int lireLDR() {
  int somme = 0;

  for (int i = 0; i < NB_LECTURES_DO; i++) {
    somme += digitalRead(LDR_PIN); // Lecture numérique : 0 (LOW) ou 1 (HIGH)
    delay(10);                     // Pause entre lectures pour stabilité
  }

  // Retourne 1 si la majorité des lectures est HIGH (≥ 3 sur 5)
  return (somme >= (NB_LECTURES_DO / 2 + 1)) ? 1 : 0;
}


// ============================================================
//  TEST LDR — Affichage de l'état courant sur le moniteur série
// ============================================================
void testLDR() {
  int val = lireLDR();

  // Mise à jour LED selon l'état au moment du test
  if (val == 1) {
    setLED(255, 255, 200); // Blanc chaud → LUMINEUX
  } else {
    setLED(20, 0, 30);     // Violet sombre → SOMBRE
  }

  Serial.println("✅ LDR LM393 fonctionnel (sortie numérique DO)");
  Serial.print("   État         : ");
  Serial.println(val == 1 ? "LUMINEUX  (DO = HIGH) → LED blanc chaud"
                          : "SOMBRE    (DO = LOW)  → LED violet foncé");
  Serial.println("   ℹ️  Ajustez la vis (potentiomètre) pour régler le seuil de détection.");
  Serial.println("      La micro LED du module s'allume au franchissement du seuil.\n");
}


// ============================================================
//  ENVOI DONNÉES — Lecture + encodage + transmission LoRaWAN
// ============================================================
void envoyerDonnees() {
  Serial.println("\n--- Envoi Données ---");

  // Lecture de l'état lumineux via vote majoritaire
  int luminosite = lireLDR(); // 0 = sombre, 1 = lumineux

  Serial.print("Luminosité : ");
  Serial.println(luminosite == 1 ? "LUMINEUX (1)" : "SOMBRE (0)");

  // Encodage du payload : 1 octet — 0x00 (sombre) ou 0x01 (lumineux)
  char payload[32];
  sprintf(payload, "AT+SEND=1:%02X", luminosite);

  Serial.print("Payload    : ");
  Serial.println(payload); // Affiche "AT+SEND=1:00" ou "AT+SEND=1:01"

  // Envoi de la trame LoRaWAN via commande AT
  envoyerAT(payload, 500);

  // Attente de la confirmation d'envoi par le réseau TTN (timeout 10s)
  unsigned long debut = millis();
  while (millis() - debut < 60000) {
    if (Serial2.available()) {
      String rep = Serial2.readStringUntil('\n');
      rep.trim();
      if (rep.length() > 0) {
        Serial.println("[RAK] " + rep);

        if (rep.indexOf("+EVT:SEND_CONFIRMED_OK") >= 0) {
          Serial.println("✅ Trame confirmée par TTN !\n");
          return;
        }
        if (rep.indexOf("+EVT:SEND_CONFIRMED_FAILED") >= 0) {
          Serial.println("❌ Confirmation TTN échouée\n");
          return;
        }
      }
    }
    delay(50);
  }

  Serial.println("⏱️ Timeout — aucune confirmation reçue dans les 10 secondes\n");
}


// ============================================================
//  ENCODAGE PAYLOAD MULTI-CAPTEURS (temp + hum + lum)
// ============================================================
/*
 * Encode trois valeurs capteurs en une chaîne hexadécimale
 * pour transmission LoRaWAN.
 *
 * Structure du payload (4 octets) :
 *   [0-1] Température  : int16_t  (×100, big-endian) — ex: 23.45°C → 0x0929
 *   [2]   Humidité     : uint8_t  (valeur entière)   — ex: 50%     → 0x32
 *   [3]   Luminosité   : uint8_t  (0 = sombre, 1 = lumineux)
 *
 * @param temp  Température en °C (ex: 23.45)
 * @param hum   Humidité en %     (ex: 50.0)
 * @param lum   État LDR          (0 ou 1)
 * @return      Chaîne hex 8 caractères (ex: "09293201")
 */
String encodePayload(float temp, float hum, int lum) {
  // --- Température : multiplication ×100 pour conserver 2 décimales ---
  int16_t tempInt = (int16_t)(temp * 100.0);
  char hexTemp[5];
  sprintf(hexTemp, "%04X", (uint16_t)tempInt);

  // --- Humidité : valeur entière 0–100, 1 octet ---
  uint8_t humInt = (uint8_t)hum;
  char hexHum[3];
  sprintf(hexHum, "%02X", humInt);

  // --- Luminosité : état binaire 0/1, 1 octet ---
  uint8_t lumInt = (uint8_t)(lum > 0 ? 1 : 0);
  char hexLum[3];
  sprintf(hexLum, "%02X", lumInt);

  // Concaténation : Temp(4) + Hum(2) + Lum(2) = 8 caractères
  return String(hexTemp) + String(hexHum) + String(hexLum);
}


// ============================================================
//  CONFIGURATION LORAWWAN
// ============================================================
void configurerLoRa() {

  // 🔄 Reset propre du module RAK
  envoyerAT("AT+RESET", 2000);
  delay(2000); // Laisse le temps au module de redémarrer

  envoyerAT("AT+NWM=1",  2500); // Mode LoRaWAN
  envoyerAT("AT+NJM=1",   500); // OTAA
  envoyerAT("AT+BAND=4",  500); // EU868
  envoyerAT("AT+DEVEUI=" + String(DEVEUI), 500);
  envoyerAT("AT+APPEUI=" + String(APPEUI), 500);
  envoyerAT("AT+APPKEY=" + String(APPKEY), 500);
  Serial.println("✅ Configuration LoRaWAN envoyée au RAK3272S");
}


// ============================================================
//  JOIN OTAA
// ============================================================
void joinOTAA() {
  Serial.println("Tentative de join OTAA (jusqu'à 60 secondes)...");
  envoyerAT("AT+JOIN=1:0:10:2", 500);

  unsigned long debut = millis();
  while (millis() - debut < 60000) {
    if (Serial2.available()) {
      String rep = Serial2.readStringUntil('\n');
      rep.trim();
      if (rep.length() > 0) {
        Serial.println("[RAK] " + rep);

        if (rep.indexOf("+EVT:JOINED") >= 0) {
          Serial.println("✅ Join OTAA RÉUSSI — connexion TTN établie\n");
          loraJoined = true;
          return;
        }
        if (rep.indexOf("+EVT:JOIN_FAILED") >= 0) {
          Serial.println("❌ Join OTAA ÉCHOUÉ — vérifiez DevEUI / AppEUI / AppKey\n");
          loraJoined = false;
          return;
        }
      }
    }
    delay(50);
  }

  Serial.println("⏱️ Timeout join. Tapez JOIN dans le moniteur série pour réessayer.\n");
}


// ============================================================
//  ENVOI COMMANDE AT
// ============================================================
String envoyerAT(String commande, unsigned long timeoutMs) {
  Serial.println("[AT] → " + commande);
  Serial2.println(commande);

  String reponse = "";
  unsigned long debut = millis();

  while (millis() - debut < timeoutMs) {
    while (Serial2.available()) {
      String ligne = Serial2.readStringUntil('\n');
      ligne.trim();
      if (ligne.length() > 0) {
        Serial.println("[AT] ← " + ligne);
        reponse += ligne + "\n";
      }
    }
    delay(20);
  }

  return reponse;
}