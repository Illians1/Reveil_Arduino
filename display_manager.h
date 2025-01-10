#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include "bluetooth_classic_manager.h"  // Pour accéder à a2dp_sink, DELAI_TIMEOUT et derniereMiseAJour

// Configuration de l'écran OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_I2C_ADDRESS 0x3C
#define PIN_DISPLAY_DURATION 30000  // 30 secondes en millisecondes

extern bool displayActive;  // État de l'écran
extern unsigned long lastActiveTime;
extern bool isDisplayingPasskey;

// Initialisation de l'objet écran
extern Adafruit_SSD1306 display;

// Initialisation de l'écran
void initDisplay();

// Mise à jour de l'affichage
void updateDisplay(const DateTime &now);

void displayPasskey(const String &passkey);

void clearPasskeyDisplay();

extern BluetoothA2DPSink a2dp_sink;
extern unsigned long DELAI_TIMEOUT;
extern unsigned long derniereMiseAJour;

// Nouvelle fonction pour activer l'écran
bool activateDisplay();

#endif