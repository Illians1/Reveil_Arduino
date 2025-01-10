#include "rtc_manager.h"
#include "display_manager.h"
#include "ble_manager.h"
#include "alarmsound_manager.h"
#include "bluetooth_classic_manager.h"
#include "SPIFFS.h"


void setup() {
  Serial.begin(115200);
  // Configuration des broches I2S pour le MAX98357A
  auto cfg = i2s.defaultConfig();
  cfg.pin_bck = 26;   // BCLK
  cfg.pin_ws = 25;    // LRC ou WS
  cfg.pin_data = 23;  // DIN
  cfg.channels = 1;   // Stéréo au lieu de mono
  cfg.sample_rate = 44100;
  cfg.bits_per_sample = 16;

  // Initialisation I2S
  i2s.begin(cfg);

  // 1. Configuration matérielle de base
  initRTC();         // Horloge
  initDisplay();     // Affichage
  initBLE();         // Bluetooth BLE
  init_bluetooth();  // Bluetooth classic
}

void loop() {
  DateTime now = rtc.now();

  handle_bluetooth();

  // Gérer l'alarme
  updateAlarm(now);

  // Gestion de l'alarme
  handleAlarmSound();

  // Mettre à jour l'écran
  updateDisplay(now);

  // Gérer les notifications liées à l'alarme
  handleAlarmNotifications();
}