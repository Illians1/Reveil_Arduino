#include "alarmsound_manager.h"

const char *startFilePath = "/";
const char *ext = "wav";
AudioSourceSPIFFS source(startFilePath, ext);
WAVDecoder decoder;
AudioPlayer player(source, i2s, decoder);

const unsigned long VOLUME_RAMP_DURATION = 30000;  // 30 secondes en millisecondes
unsigned long alarmStartTime = 0;
float currentVolume = 0.0;  // Volume entre 0.0 et 1.0

void stopAlarmSound() {
  player.stop();
  currentVolume = 0.0;
}

void handleAlarmSound() {
  if (myAlarm.triggered) {
    // Déconnecter le Bluetooth si actif
    if (a2dp_sink.is_connected()) {
      a2dp_sink.disconnect();
      delay(100);
    }
    
    // Initialiser le temps de départ si c'est le début de l'alarme
    if (!player.isActive()) {
      source.begin();
      player.begin();
      alarmStartTime = millis();
      currentVolume = 0.1;
      Serial.println("Démarrage de l'alarme");
    }
    
    // Calculer le volume progressif
    unsigned long elapsedTime = millis() - alarmStartTime;
    if (elapsedTime < VOLUME_RAMP_DURATION) {
      currentVolume = (float)elapsedTime / VOLUME_RAMP_DURATION;
    } else {
      currentVolume = 1.0;
    }
    
    // Appliquer le volume
    player.setVolume(currentVolume);
    
    // Gérer la lecture du son
    size_t bytes_copied = player.copy();
    if (bytes_copied == 0) {  // Si aucun octet n'a été copié, la lecture est terminée
      // Relancer la lecture en gardant le même volume
      source.begin();
      player.begin();
      player.setVolume(currentVolume);
      Serial.printf("Relance de l'alarme (Volume: %.2f)\n", currentVolume);
    }
  }
}