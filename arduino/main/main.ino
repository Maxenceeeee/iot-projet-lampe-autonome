/*
 * Projet Final IoT -- Module 5
 * Mastere Expert Dev Logiciel / Mobile / IoT -- Ynov Campus 2025-2026
 *
 * Description : Lampe autonome connectée -- ESP32 + RAK3172 + Module de photorésistance LDR LDR LM393
 *               Lecture de l'état lumineux (sombre / lumineux) via un module photorésistance LDR LM393 à sortie numérique (DO), et envoi LoRaWAN OTAA vers TTN toutes les 60 secondes.
 *                   Transmission LoRaWAN vers TTN -> Node-RED -> InfluxDB -> Grafana
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

// ---------------------------------------------------------------------------
// Includes
// ---------------------------------------------------------------------------
#include "config.h"

// TODO : inclure la bibliothèque de votre capteur
// Aucune bibliothèque spécifique pour une photorésistance LDR -- lecture analogique directe 

// ---------------------------------------------------------------------------
// Variables globales persistantes (RTC Memory -- survivent au Deep Sleep)
// ---------------------------------------------------------------------------
RTC_DATA_ATTR int boot_count = 0; // Compteur de cycles (diagnostic)
bool loraJoined  = false; // Statut de la connexion OTAA
unsigned long dernierEnvoi = 0; // Horodatage du dernier envoi (ms)

// ---------------------------------------------------------------------------
// Prototypes de fonctions
// ---------------------------------------------------------------------------
void setLED(uint8_t r, uint8_t g, uint8_t b) {
  ledcWrite(LED_R, r);
  ledcWrite(LED_G, g);
  ledcWrite(LED_B, b);
}

// ---------------------------------------------------------------------------
// setup() -- Execute a chaque reveil (Deep Sleep = reset complet)
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// loop() -- Jamais atteint (Deep Sleep provoque un réinitialisation)
// ---------------------------------------------------------------------------
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

// ===========================================================================
// FONCTIONS METIER -- A implementer
// ===========================================================================

/**
 * lireLDR()
 * Lit la valeur du capteur LDR et retourne 1 si lumineux, 0 si sombre.
 */
int lireLDR() {
  int somme = 0;

  for (int i = 0; i < NB_LECTURES_DO; i++) {
    somme += digitalRead(LDR_PIN); // Lecture numérique : 0 (LOW) ou 1 (HIGH)
    delay(10);                     // Pause entre lectures pour stabilité
  }

  // Retourne 1 si la majorité des lectures est HIGH (≥ 3 sur 5)
  return (somme >= (NB_LECTURES_DO / 2 + 1)) ? 0 : 1;
}

/**
 * testLDR() — Affichage de l'état courant sur le moniteur série
 */
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

/**
 * envoyerDonnees()
 * Lit l'état du capteur LDR, encode le payload, et envoie la trame LoRaWAN via le RAK3172S.
 * Affiche les étapes et résultats sur le moniteur série.
 */
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
  while (millis() - debut < 10000) {
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

/**
 * joinOTAA()
 * Tente de rejoindre le réseau LoRaWAN en mode OTAA.
 * Affiche les étapes et résultats sur le moniteur série.
 */
void joinOTAA() {
  Serial.println("Tentative de join OTAA (jusqu'à 60 secondes)...");
  envoyerAT("AT+JOIN=1:0:10:8", 500);

  unsigned long debut = millis();
  while (millis() - debut < 5000) {
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

/**
 * configurerLoRa()
 * Envoie les commandes AT de configuration LoRaWAN au RAK3272S.
 * Configure le mode, les paramètres OTAA, et les clés d'identification.
 */
void configurerLoRa() {
  envoyerAT("AT+NWM=1",  2500); // Mode LoRaWAN
  envoyerAT("AT+NJM=1",   500); // OTAA
  envoyerAT("AT+BAND=4",  500); // EU868
  envoyerAT("AT+DEVEUI=" + String(DEVEUI), 500);
  envoyerAT("AT+APPEUI=" + String(APPEUI), 500);
  envoyerAT("AT+APPKEY=" + String(APPKEY), 500);
  Serial.println("✅ Configuration LoRaWAN envoyée au RAK3272S");
}

/**
 * envoyerAT()
 * Envoie une commande AT au RAK3172S et attend une réponse.
 * @param commande La commande AT à envoyer.
 * @param timeoutMs Le délai d'attente en millisecondes.
 * @return La réponse reçue du RAK3172S.
 */
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
