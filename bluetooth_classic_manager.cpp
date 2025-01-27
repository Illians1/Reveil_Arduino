#include "bluetooth_classic_manager.h"

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

// Constantes pour les boutons et délais
const int BUTTON_INCREASE_PIN = 32;
const int BUTTON_DECREASE_PIN = 33;
const int BUTTON_NEXT_PIN = 16;
const int BUTTON_PREV_PIN = 17;

// Pins pour l'encodeur rotatif KY-040
const int PIN_CLK = 25;    // Pin CLK de l'encodeur
const int PIN_DT = 26;     // Pin DT de l'encodeur
const int PIN_SW = 27;     // Pin SW (bouton) de l'encodeur

// Variables pour l'encodeur
volatile int lastCLK;      // Dernier état de CLK
volatile int volumeLevel = 50;  // Volume initial (0-100)

const unsigned long TIMEOUT_STEP = 300000;  // 5 minutes en millisecondes
const unsigned long MAX_TIMEOUT = 7200000;  // 120 minutes en millisecondes
const unsigned long DELAI_GRACE = 300000;   // 5 minutes de grâce en millisecondes
unsigned long DELAI_TIMEOUT = 1800000;      // 30 minutes en millisecondes (valeur initiale)

// Variables pour gérer le timeout
unsigned long derniereMiseAJour = 0;
unsigned long temps = 0;
unsigned long delai = DELAI_TIMEOUT;

// Fonction callback appelée lors des changements d'état de connexion
void gestion_connexion(esp_a2d_connection_state_t state, void *ptr) {
  if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
    Serial.println("Appareil connecté");
    delai = DELAI_TIMEOUT;
    derniereMiseAJour = millis();
  } else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
    Serial.println("Appareil déconnecté");
  }
}

void init_bluetooth() {
  // Configuration des boutons
  
  pinMode(BUTTON_INCREASE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DECREASE_PIN, INPUT_PULLUP);

  // Configuration des nouveaux boutons
  pinMode(BUTTON_NEXT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PREV_PIN, INPUT_PULLUP);

  // Définir la fonction de callback pour détecter la connexion
  a2dp_sink.set_on_connection_state_changed(gestion_connexion);

  // Configuration du contrôle AVRCP
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);

  // Démarrage du récepteur Bluetooth
  a2dp_sink.start("ESP32_Audio");

  // Obtenir et afficher l'adresse Bluetooth
  const uint8_t *adresse = esp_bt_dev_get_address();
  Serial.print("Adresse Bluetooth: ");
  for (int i = 0; i < 6; i++) {
    char str[3];
    sprintf(str, "%02X", adresse[i]);
    Serial.print(str);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  
/*
  // Configuration de l'encodeur rotatif
  pinMode(PIN_CLK, INPUT);
  pinMode(PIN_DT, INPUT);
  pinMode(PIN_SW, INPUT_PULLUP);
  
  lastCLK = digitalRead(PIN_CLK);  // Lire l'état initial
  */
}

void handle_bluetooth() {
  // Gestion des boutons
 
  if (digitalRead(BUTTON_INCREASE_PIN) == LOW) {
    if (!activateDisplay()) {
      delay(200);
      return;
    }

    if (a2dp_sink.is_connected()) {
      if (delai == 0) {
        delai = TIMEOUT_STEP;
      } else if (delai < MAX_TIMEOUT) {  // Vérifier si on n'a pas atteint le maximum
        delai += TIMEOUT_STEP;
      }
      Serial.printf("Timeout augmenté à %d minutes\n", delai / 60000);
    }
    delay(200);
  }

  if (digitalRead(BUTTON_DECREASE_PIN) == LOW) {
    if (!activateDisplay()) {
      delay(200);
      return;
    }

    // Ne modifier le délai que si un appareil est connecté
    if (a2dp_sink.is_connected()) {
      if (delai <= TIMEOUT_STEP) {
        delai = 0;
      } else {
        delai -= TIMEOUT_STEP;
        Serial.printf("Timeout réduit à %d minutes\n", delai / 60000);
      }
    }
    delay(200);
  }

  // Gestion des boutons de contrôle de la musique
  if (digitalRead(BUTTON_NEXT_PIN) == LOW) {

    if (a2dp_sink.is_connected()) {
      a2dp_sink.next();
    }
    delay(200);
  }

  if (digitalRead(BUTTON_PREV_PIN) == LOW) {

    if (a2dp_sink.is_connected()) {
      a2dp_sink.previous();
    }
    delay(200);
  }

  // Vérifier si un appareil est connecté et si le délai est dépassé
  if (a2dp_sink.is_connected()) {
    unsigned long tempsEcoule = millis() - derniereMiseAJour;
    if (tempsEcoule >= (delai + DELAI_GRACE)) {
      Serial.println("Timeout + délai de grâce atteint - Déconnexion");
      a2dp_sink.disconnect();
      derniereMiseAJour = millis();
    }
  }
/*
  // Gestion de l'encodeur rotatif
  if (a2dp_sink.is_connected()) {
    // Gestion de la rotation pour le volume
    int clkState = digitalRead(PIN_CLK);
    if (clkState != lastCLK) {
      if (digitalRead(PIN_DT) != clkState) {
        // Rotation horaire
        if (volumeLevel < 100) {
          volumeLevel += 5;
          a2dp_sink.set_volume(volumeLevel);
        }
      } else {
        // Rotation anti-horaire
        if (volumeLevel > 0) {
          volumeLevel -= 5;
          a2dp_sink.set_volume(volumeLevel);
        }
      }
      Serial.printf("Volume: %d%%\n", volumeLevel);
    }
    lastCLK = clkState;

    // Gestion du bouton pour play/pause
    static bool isPlaying = true;  // Pour suivre l'état de lecture
    if (digitalRead(PIN_SW) == LOW) {
      if (!activateDisplay()) {
        delay(200);
        return;
      }
      
      if (isPlaying) {
        a2dp_sink.pause();
        isPlaying = false;
        Serial.println("Pause");
      } else {
        a2dp_sink.play();
        isPlaying = true;
        Serial.println("Play");
      }
      delay(200);  // Debounce
    }
  }
  */
}

// Callback optionnel pour recevoir les métadonnées de la piste en cours
void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  switch (id) {
    case ESP_AVRC_MD_ATTR_TITLE:
      Serial.printf("Titre: %s\n", text);
      break;
    case ESP_AVRC_MD_ATTR_ARTIST:
      Serial.printf("Artiste: %s\n", text);
      break;
    case ESP_AVRC_MD_ATTR_ALBUM:
      Serial.printf("Album: %s\n", text);
      break;
  }
}