#include <Wire.h>
#include "rtc_manager.h"
#include "ble_manager.h"
#include "display_manager.h"
#include "buzzer_manager.h"

void setup() {
  Serial.begin(115200);

  // Initialiser les composants
  initRTC();
  initDisplay();
  initBLE();
  initBuzzer();
}

void loop() {
  DateTime now = rtc.now();

  // Gérer l'alarme
  updateAlarm(now);

  if (shouldTriggerAlarm(now)) {
    stopAlarm();
    stopBuzzer();
  }

  // Gestion du buzzer
  handleBuzzer();

  // Mettre à jour l'écran
  updateDisplay(now);

  // Gérer les notifications liées à l'alarme
  handleAlarmNotifications();

}