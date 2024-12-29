#include "buzzer_manager.h"
#include "rtc_manager.h"

bool buzzerOn = false; // État du buzzer
unsigned long lastBuzzerToggle = 0; // Temps de la dernière bascule

void initBuzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void handleBuzzer() {
  if (myAlarm.triggered) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastBuzzerToggle >= BUZZER_INTERVAL) {
      lastBuzzerToggle = currentMillis;
      buzzerOn = !buzzerOn; // Alterner entre ON et OFF
      if (buzzerOn) {
        tone(BUZZER_PIN, BUZZER_FREQUENCY); // Activer le buzzer à la fréquence définie
      } else {
        noTone(BUZZER_PIN); // Désactiver le buzzer
      }
    }
  }
}

void stopBuzzer() {
  noTone(BUZZER_PIN);
}