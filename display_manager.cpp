#include "display_manager.h"
#include "rtc_manager.h"

// Création de l'objet écran OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

bool displayActive = true;  // Par défaut, l'écran est éteint
const unsigned long screenTimeout = 10000;  // Délai avant extinction (10 secondes)
bool isDisplayingPasskey = false;           // Indique si on affiche uniquement le code PIN
String currentPasskey = "";                 // Stocke le code PIN affiché
unsigned long lastActiveTime = 0;

// Fonction pour activer l'écran
bool activateDisplay() {
  if (!displayActive) {
    displayActive = true;
    lastActiveTime = millis();
    display.ssd1306_command(SSD1306_DISPLAYON);
    return false;  // Indique que l'action du bouton nce doit pas être traitée
  }
  return true;  // Indique que l'action du bouton peut être traitée
}

void initDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println("Erreur : Écran OLED non détecté !");
    while (true);
  }
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();
}

// Affiche uniquement le code PIN
void displayPasskey(const String &passkey) {
  currentPasskey = passkey;
  isDisplayingPasskey = true;
  displayActive = true;

  display.ssd1306_command(SSD1306_DISPLAYON);
  display.clearDisplay();

  // Centrer le texte
  display.setTextSize(1);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds("Code PIN:", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 0);
  display.print("Code PIN:");

  // Afficher le code PIN en grand et centré
  display.setTextSize(3);  // Taille plus grande pour le PIN
  display.getTextBounds(passkey.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 16);
  display.print(passkey);

  display.display();
}

// Réinitialise l'écran après l'affichage du code PIN
void clearPasskeyDisplay() {
  isDisplayingPasskey = false;
  
  // Réinitialiser l'affichage
  display.clearDisplay();
  display.setTextSize(1);  // Retour à la taille de texte par défaut
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.display();
  
  // Optionnel : éteindre l'écran après avoir effacé le code
  displayActive = false;
  display.ssd1306_command(SSD1306_DISPLAYOFF);
}

void updateDisplay(const DateTime &now) {
  // Vérifier si le temps d'affichage du PIN est écoulé
  if (isDisplayingPasskey && (millis() - lastActiveTime > PIN_DISPLAY_DURATION)) {
    isDisplayingPasskey = false;
    displayActive = false;
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }

  if (isDisplayingPasskey) {
    return;
  }

  // Vérifier le timeout de l'écran
  if (displayActive && (millis() - lastActiveTime > screenTimeout)) {
    displayActive = false;
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }

  // Ne pas mettre à jour l'affichage si l'écran est éteint
  if (!displayActive) {
    return;
  }

  display.clearDisplay();

  // Ligne 1 : Heure actuelle
  display.setCursor(0, 0);
  if (timeSet) {
    display.print("Heure: ");
    display.printf("%02d:%02d", now.hour(), now.minute());
  } else {
    display.print("Heure non initiee");
  }

  // Ligne 2 : État de l'alarme (version condensée)
  display.setCursor(0, 10);
  if (myAlarm.set) {
    display.printf("Alarme: %02d:%02d", myAlarm.hour, myAlarm.minute);
    if (myAlarm.triggered) {
      display.print(" [!]");
    }
  } else {
    display.print("Alarme: Off");
  }

  // Ligne 3 : État Bluetooth
  display.setCursor(0, 20);
  if (a2dp_sink.is_connected()) {
    if (delai == 0) {
      display.print("BT: Connecte (Off)");
    } else {
      unsigned long tempsEcoule = millis() - derniereMiseAJour;
      unsigned long tempsRestant = (tempsEcoule >= delai) ? 0 : (delai - tempsEcoule);
      unsigned long minutesRestantes = tempsRestant / 60000;
      display.printf("BT: %02lu min", minutesRestantes);
    }
  } else {
    display.print("BT: Deconnecte");
  }

  display.display();
}