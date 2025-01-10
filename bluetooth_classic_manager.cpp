#include "bluetooth_classic_manager.h"

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

// Constantes pour les boutons et délais
const int BUTTON_INCREASE_PIN = 32;
const int BUTTON_DECREASE_PIN = 33;
const int BUTTON_NEXT_PIN = 16;
const int BUTTON_PREV_PIN = 17;
const unsigned long TIMEOUT_STEP = 300000;     // 5 minutes en millisecondes
const unsigned long MAX_TIMEOUT = 7200000;     // 120 minutes en millisecondes
unsigned long DELAI_TIMEOUT = 1800000;         // 30 minutes en millisecondes (valeur initiale)

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
  const uint8_t* adresse = esp_bt_dev_get_address();
  Serial.print("Adresse Bluetooth: ");
  for (int i = 0; i < 6; i++) {
    char str[3];
    sprintf(str, "%02X", adresse[i]);
    Serial.print(str);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
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
        Serial.println("Timeout désactivé - Déconnexion forcée");
        a2dp_sink.disconnect();
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
    if (delai == 0) {
      // Forcer la déconnexion si le timeout est à 0
      Serial.println("Timeout désactivé - Déconnexion forcée");
      a2dp_sink.disconnect();
    } else {
      unsigned long tempsEcoule = millis() - derniereMiseAJour;
      unsigned long tempsRestant = (tempsEcoule >= delai) ? 0 : (delai - tempsEcoule);

      if (tempsEcoule >= delai) {
        Serial.println("Timeout atteint - Déconnexion");
        a2dp_sink.disconnect();
        derniereMiseAJour = millis();
      }
    }
  }
}

// Callback optionnel pour recevoir les métadonnées de la piste en cours
void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  switch(id) {
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