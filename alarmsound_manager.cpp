#include "alarmsound_manager.h"

const char *startFilePath = "/";
const char *ext = "wav";
AudioSourceSPIFFS source(startFilePath, ext);
WAVDecoder decoder;
AudioPlayer player(source, i2s, decoder);

void stopAlarmSound() {
  player.stop();
}

void handleAlarmSound() {
  if (myAlarm.triggered) {
    // Déconnecter le Bluetooth si actif
    if (a2dp_sink.is_connected()) {
      a2dp_sink.disconnect();
      delay(100);
    }
    
    // Démarrer la lecture si elle n'est pas déjà active
    if (!player.isActive()) {
      source.begin();
      player.begin();
      Serial.println("Démarrage de l'alarme");
    }
    
    size_t bytes_copied = player.copy();
    if (bytes_copied == 0) {  // Si aucun octet n'a été copié, la lecture est terminée
      // Relancer la lecture
      source.begin();
      player.begin();
      Serial.println("Relance de l'alarme");
    }
  }
}