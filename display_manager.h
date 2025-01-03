#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>

// Configuration de l'écran OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_I2C_ADDRESS 0x3C
#define BUTTON_PIN 15  // Remplacez par le GPIO utilisé pour le bouton

extern bool displayActive;  // État de l'écran

// Initialisation de l'objet écran
extern Adafruit_SSD1306 display;

// Initialisation de l'écran
void initDisplay();

// Mise à jour de l'affichage
void updateDisplay(const DateTime &now);

void displayPasskey(const String &passkey);

void clearPasskeyDisplay();



#endif