#include "display_manager.h"
#include "rtc_manager.h"

// Création de l'objet écran OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

bool displayActive = true;  // Par défaut, l'écran est éteint
const unsigned long screenTimeout = 10000;  // Délai avant extinction (10 secondes)
bool isDisplayingPasskey = false;           // Indique si on affiche uniquement le code PIN
String currentPasskey = "";                 // Stocke le code PIN affiché
unsigned long lastActiveTime = 0;

void initDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println("Erreur : Écran OLED non détecté !");
    while (true)
      ;  // Boucle infinie en cas d'échec
  }
  pinMode(BUTTON_PIN, INPUT_PULLUP);            // Initialisation du bouton avec pull-up
  display.ssd1306_command(SSD1306_DISPLAYOFF);  // Éteindre l'écran au démarrage
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();
}

// Affiche uniquement le code PIN
void displayPasskey(const String &passkey) {
  currentPasskey = passkey;
  isDisplayingPasskey = true;

  display.ssd1306_command(SSD1306_DISPLAYON);
  display.clearDisplay();

  // Afficher le message d'attente de confirmation
  display.setCursor(0, 0);
  display.print("Confirmez le code:");

  // Afficher le code PIN
  display.setCursor(0, 16);
  display.setTextSize(2);  // Agrandir le texte pour une meilleure lisibilité
  display.printf("%s", passkey.c_str());
  display.setTextSize(1);  // Réinitialiser la taille du texte

  display.display();
}

// Réinitialise l'écran après l'affichage du code PIN
void clearPasskeyDisplay() {
  isDisplayingPasskey = false;
  currentPasskey = "";
  display.clearDisplay();
  display.display();
}

void updateDisplay(const DateTime &now) {
  if (isDisplayingPasskey) {
    // Si on est en mode affichage de code, ne rien afficher d'autre
    return;
  }

  if (digitalRead(BUTTON_PIN) == LOW) {
    displayActive = true;
    lastActiveTime = millis();  // Réinitialise le délai
    display.ssd1306_command(SSD1306_DISPLAYON);
  }
  if (displayActive && (millis() - lastActiveTime > screenTimeout)) {
    displayActive = false;
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }

  display.clearDisplay();

  // Afficher l'heure actuelle
  display.setCursor(0, 0);
  if (timeSet) {
    display.print("Heure actuelle: ");
    display.printf("%02d:%02d", now.hour(), now.minute());
  } else {
    display.print("Heure non initiee");
  }

  // Afficher l'état de l'alarme
  display.setCursor(0, 16);
  if (myAlarm.set) {
    if (myAlarm.triggered) {
      display.print("Alarme: En cours...");
    } else {
      display.print("Alarme active a: ");
      display.setCursor(0, 32);  // Ligne suivante
      display.printf("%02d:%02d", myAlarm.hour, myAlarm.minute);
    }
  } else {
    display.print("Alarme: Desactivee");
  }

  display.display();
}