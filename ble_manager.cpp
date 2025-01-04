#include "ble_manager.h"

// UUIDs BLE
#define SERVICE_UUID "c25bc52e-8920-407f-ae4f-ff03c471d222"
#define CURRENT_TIME_UUID "49fd0f2a-2262-4eeb-b23e-c5574fd8cafc"
#define ALARM_TIME_UUID "ad350e7c-9855-4ad8-84e3-6821cd211062"
#define ALARM_CONTROL_UUID "da413e28-8ea1-4193-8e1c-eacdb2e526a0"
#define TASK_STATUS_UUID "f7b3034a-97ec-4d3a-8c52-2a66b19f69c5"    // UUID pour la notification
#define ALARM_STATE_UUID "e53b49b6-90ea-411c-a1a3-0401fe5525d2"    // UUID pour l'état de l'alarme
#define ALARM_DISABLE_UUID "80bb916a-d62e-423a-8d8f-73d1b42ebe3b"  // UUID pour désactiver l'alarme

unsigned long lastNotificationTime = 0;  // Initialisé à 0

BLECharacteristic *taskStatusChar;

// --- CALLBACKS BLE --- //
class CurrentTimeCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String value = pCharacteristic->getValue();
    if (value.length() == 19) {  // Format attendu : "YYYY-MM-DD HH:MM:SS"
      int year, month, day, hour, minute, second;
      sscanf(value.c_str(), "%4d-%2d-%2d %2d:%2d:%2d", &year, &month, &day, &hour, &minute, &second);
      DateTime newTime(year, month, day, hour, minute, second);
      setRTC(newTime);  // Utilise la fonction pour régler l'heure du RTC
      timeSet = true;

      // Activer l'écran
      displayActive = true;
      lastActiveTime = millis();
      display.ssd1306_command(SSD1306_DISPLAYON);

      Serial.println("Heure actuelle synchronisée !");
      // Envoyer une notification à l'application
      if (taskStatusChar) {  // Vérifiez que la caractéristique de notification est initialisée
        taskStatusChar->setValue("Heure Maj");
        taskStatusChar->notify();  // Envoie la notification
        Serial.println("Notification envoyée : Heure Maj");
      }
    }
  }
};

class AlarmTimeCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String value = pCharacteristic->getValue();
    if (value.length() == 5) {  // Format attendu : "HH:MM"
      sscanf(value.c_str(), "%2d:%2d", &myAlarm.hour, &myAlarm.minute);
      myAlarm.set = true;

      // Activer l'écran
      displayActive = true;
      lastActiveTime = millis();
      display.ssd1306_command(SSD1306_DISPLAYON);

      Serial.printf("Alarme configurée à %02d:%02d\n", myAlarm.hour, myAlarm.minute);
      if (taskStatusChar) {
        taskStatusChar->setValue("Alarme Maj");
        taskStatusChar->notify();
        Serial.println("Notification envoyée : Alarme Maj");
      }
    }
  }

  void onRead(BLECharacteristic *pCharacteristic) override {
    char alarmTime[6];  // Format "HH:MM\0"
    if (myAlarm.set) {
      sprintf(alarmTime, "%02d:%02d", myAlarm.hour, myAlarm.minute);
      pCharacteristic->setValue((uint8_t *)alarmTime, strlen(alarmTime));  // Correction du setValue
      Serial.printf("Lecture de l'heure de l'alarme : %s\n", alarmTime);
    } else {
      pCharacteristic->setValue((uint8_t *)"", 0);  // Renvoyer une chaîne vide si l'alarme n'est pas configurée
      Serial.println("Lecture de l'heure de l'alarme : Alarme non configurée");
    }
  }
};

class AlarmControlCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String value = pCharacteristic->getValue();
    if (value == "STOP") {
      if (myAlarm.triggered) {
        stopAlarm();
        stopAlarmSound();
        // Envoyer une notification à l'application
        if (taskStatusChar) {  // Vérifiez que la caractéristique de notification est initialisée
          taskStatusChar->setValue("Alarme Stop");
          taskStatusChar->notify();  // Envoie la notification
          Serial.println("Notification envoyée : Alarme Stop");
        }
      }
    } else Serial.println("Commande invalide pour stopper l'alarme");
  }
};

class AlarmDisableCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String value = pCharacteristic->getValue();
    if (value == "DISABLE") {  // Vérifiez que la commande est valide
      if (myAlarm.set) {
        myAlarm.set = false;  // Désactiver l'alarme

        // Activer l'écran
        displayActive = true;
        lastActiveTime = millis();
        display.ssd1306_command(SSD1306_DISPLAYON);

        Serial.println("Commande reçue : Alarme désactivée");
        if (taskStatusChar) {  // Envoyer une notification
          taskStatusChar->setValue("Alarme désactivée");
          taskStatusChar->notify();
          Serial.println("Notification envoyée : Alarme désactivée");
        }
      }
    } else {
      Serial.println("Commande invalide pour désactiver l'alarme");
    }
  }
};

// --- CALLBACKS DU SERVEUR --- //
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    Serial.println("Appareil connecté.");
  }

  void onDisconnect(BLEServer *server) override {
    Serial.println("Appareil déconnecté.");
    server->startAdvertising();  // Redémarre la publicité après une déconnexion
  }
};

void handleAlarmNotifications() {
  unsigned long currentMillis = millis();

  // Vérifiez si l'alarme est déclenchée
  if (myAlarm.triggered) {
    // Vérifiez si 5 minutes se sont écoulées depuis la dernière notification
    if (currentMillis - lastNotificationTime >= 5000) {  // 5000 ms = 5 secondes
      if (taskStatusChar) {                              // Vérifiez que la caractéristique est initialisée
        taskStatusChar->setValue("Alarme en cours");
        taskStatusChar->notify();  // Envoie la notification
        Serial.println("Notification envoyée : Alarme en cours");
      }
      lastNotificationTime = currentMillis;  // Mettez à jour le dernier temps d'envoi
    }
  } else {
    // Réinitialisez le dernier temps si l'alarme n'est pas déclenchée
    lastNotificationTime = currentMillis;
  }
}

void initBLE() {
  BLEDevice::init("AuroWake_1234");


  // Configurer la sécurité BLE
  BLESecurity *pSecurity = new BLESecurity();

  // Activer le mode bonding
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);                           // Mode bonding avec Secure Connections
  pSecurity->setCapability(ESP_IO_CAP_NONE);                                    // Affichage du code sur l'ESP32
  pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);  // Chiffrement et clé d'identité

  BLEServer *server = BLEDevice::createServer();

  // Ajouter les callbacks
  server->setCallbacks(new ServerCallbacks());

  // Créer le service avec une taille suffisante
  BLEService *service = server->createService(BLEUUID(SERVICE_UUID), 30, 0);  // Augmenter le nombre de handles

  taskStatusChar = service->createCharacteristic(
    TASK_STATUS_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);
  BLE2902 *taskStatusDesc = new BLE2902();
  taskStatusDesc->setNotifications(true);
  taskStatusChar->addDescriptor(taskStatusDesc);

  // Current Time
  BLECharacteristic *currentTimeChar = service->createCharacteristic(
    CURRENT_TIME_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  currentTimeChar->setCallbacks(new CurrentTimeCallback());
  BLE2902 *currentTimeDesc = new BLE2902();
  currentTimeDesc->setNotifications(true);
  currentTimeChar->addDescriptor(currentTimeDesc);
  currentTimeChar->addDescriptor(new BLEDescriptor(BLEUUID((uint16_t)0x2901)));

  // Alarm Time
  BLECharacteristic *alarmTimeChar = service->createCharacteristic(
    ALARM_TIME_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  alarmTimeChar->setCallbacks(new AlarmTimeCallback());
  BLE2902 *alarmTimeDesc = new BLE2902();
  alarmTimeDesc->setNotifications(true);
  alarmTimeChar->addDescriptor(alarmTimeDesc);
  alarmTimeChar->addDescriptor(new BLEDescriptor(BLEUUID((uint16_t)0x2901)));

  // Alarm Control
  BLECharacteristic *alarmControlChar = service->createCharacteristic(
    ALARM_CONTROL_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  alarmControlChar->setCallbacks(new AlarmControlCallback());
  BLE2902 *alarmControlDesc = new BLE2902();
  alarmControlDesc->setNotifications(true);
  alarmControlChar->addDescriptor(alarmControlDesc);
  alarmControlChar->addDescriptor(new BLEDescriptor(BLEUUID((uint16_t)0x2901)));

  // Alarm Disable - Vérification
  BLECharacteristic *alarmDisableChar = service->createCharacteristic(
    ALARM_DISABLE_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  if (alarmDisableChar == nullptr) {
    Serial.println("Erreur lors de la création de alarmDisableChar");
  } else {
    alarmDisableChar->setCallbacks(new AlarmDisableCallback());
    BLE2902 *alarmDisableDesc = new BLE2902();
    alarmDisableDesc->setNotifications(true);
    alarmDisableChar->addDescriptor(alarmDisableDesc);
    Serial.println("AlarmDisable caractéristique créée avec succès");
  }

  // Démarrer le service AVANT de démarrer l'advertising
  service->start();

  // Configuration de l'advertising
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMinPreferred(0x12);

  // Démarrer l'advertising
  BLEDevice::startAdvertising();
  Serial.println("BLE prêt à se connecter");
}