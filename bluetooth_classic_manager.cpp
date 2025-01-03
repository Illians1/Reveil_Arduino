#include "bluetooth_classic_manager.h"

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

// Variables pour gérer le timeout
unsigned long derniereMiseAJour = 0;
unsigned long temps = 0;
const unsigned long DELAI_TIMEOUT = 60000;  // 1 minute en millisecondes

// Fonction callback appelée lors des changements d'état de connexion
void gestion_connexion(esp_a2d_connection_state_t state, void *ptr) {
  if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
    Serial.println("Appareil connecté");
    derniereMiseAJour = millis();
  } else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
    Serial.println("Appareil déconnecté");
  }
}

void init_bluetooth() {
  // Définir la fonction de callback pour détecter la connexion
  a2dp_sink.set_on_connection_state_changed(gestion_connexion);

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
  // Vérifier si un appareil est connecté et si le délai est dépassé
  if (a2dp_sink.is_connected()) {
      temps = millis() - derniereMiseAJour;
      Serial.println(temps);
    if (millis() - derniereMiseAJour >= DELAI_TIMEOUT) {
      Serial.println("Timeout atteint - Déconnexion");
      a2dp_sink.disconnect();
      derniereMiseAJour = millis();
    }
  }
}