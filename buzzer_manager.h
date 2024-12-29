#ifndef BUZZER_MANAGER_H
#define BUZZER_MANAGER_H

#include <Arduino.h>

// Broche du buzzer
#define BUZZER_PIN 18

// Fréquence et intervalle du buzzer
#define BUZZER_FREQUENCY 1000
#define BUZZER_INTERVAL 500 // En millisecondes

// Initialisation du buzzer
void initBuzzer();

extern unsigned long lastToggleTime; 
extern bool buzzerState;

// Gestion du son du buzzer (alternance ON/OFF)
void handleBuzzer();

// Désactiver le buzzer
void stopBuzzer();

#endif