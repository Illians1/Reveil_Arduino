#include "ble_manager.h"
#include "rtc_manager.h"
#include "buzzer_manager.h"
#include "display_manager.h"

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
      Serial.printf("Alarme configurée à %02d:%02d\n", myAlarm.hour, myAlarm.minute);
      // Envoyer une notification à l'application
      if (taskStatusChar) {  // Vérifiez que la caractéristique de notification est initialisée
        taskStatusChar->setValue("Alarme Maj");
        taskStatusChar->notify();  // Envoie la notification
        Serial.println("Notification envoyée : Alarme Maj");
      }
    }
  }
};

class AlarmControlCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String value = pCharacteristic->getValue();
    if (value == "STOP") {
      stopAlarm();
      stopBuzzer();
      // Envoyer une notification à l'application
      if (taskStatusChar) {  // Vérifiez que la caractéristique de notification est initialisée
        taskStatusChar->setValue("Alarme Stop");
        taskStatusChar->notify();  // Envoie la notification
        Serial.println("Notification envoyée : Alarme Stop");
      }
    }
  }
};

class AlarmStateCallback : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic) override {
    char alarmData[32];
    if (myAlarm.set) {
      sprintf(alarmData, "%d:%02d %s",
              myAlarm.hour, myAlarm.minute,
              myAlarm.set ? "Set" : "Not Set");
      if (taskStatusChar) {  // Vérifiez que la caractéristique de notification est initialisée
      }
    }
    pCharacteristic->setValue(alarmData);
    Serial.printf("Lecture de l'état de l'alarme : %s\n", alarmData);
  }
};

class AlarmDisableCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String value = pCharacteristic->getValue();
    if (value == "DISABLE") {  // Vérifiez que la commande est valide
      myAlarm.set = false;     // Désactiver l'alarme
      Serial.println("Commande reçue : Alarme désactivée");
      if (taskStatusChar) {  // Envoyer une notification
        taskStatusChar->setValue("Alarme désactivée");
        taskStatusChar->notify();
        Serial.println("Notification envoyée : Alarme désactivée");
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

  BLEService *service = server->createService(SERVICE_UUID);

  taskStatusChar = service->createCharacteristic(
    TASK_STATUS_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);
  // Ajout explicite du descripteur CCCD pour permettre les notifications
  BLE2902 *desc2902 = new BLE2902();
  // Descripteur CCCD pour permettre l'activation des notifications
  desc2902->setNotifications(true);  // Active les notifications par défaut côté ESP32
  taskStatusChar->addDescriptor(desc2902);

  // Heure actuelle
  BLECharacteristic *currentTimeChar = service->createCharacteristic(
    CURRENT_TIME_UUID, BLECharacteristic::PROPERTY_WRITE);
  currentTimeChar->setCallbacks(new CurrentTimeCallback());
  // Créer un descripteur
  BLEDescriptor *currentTimeDesc = new BLEDescriptor(BLEUUID((uint16_t)0x2901));  // BLE2901 standard
  currentTimeDesc->setValue("Current Time");                                      // Description textuelle
  currentTimeChar->addDescriptor(currentTimeDesc);
    BLE2902 *currDesc2902 = new BLE2902();
  // Descripteur CCCD pour permettre l'activation des notifications
  currDesc2902->setNotifications(true);  // Active les notifications par défaut côté ESP32
  currentTimeChar->addDescriptor(currDesc2902);

  // Heure de l'alarme
  BLECharacteristic *alarmTimeChar = service->createCharacteristic(
    ALARM_TIME_UUID, BLECharacteristic::PROPERTY_WRITE);
  alarmTimeChar->setCallbacks(new AlarmTimeCallback());
  // Créer un descripteur
  BLEDescriptor *alarmTimeDesc = new BLEDescriptor(BLEUUID((uint16_t)0x2901));  // BLE2901 standard
  alarmTimeDesc->setValue("Alarm Time");                                        // Description textuelle
  alarmTimeChar->addDescriptor(alarmTimeDesc);

  // Contrôle de l'alarme
  BLECharacteristic *alarmControlChar = service->createCharacteristic(
    ALARM_CONTROL_UUID, BLECharacteristic::PROPERTY_WRITE);
  alarmControlChar->setCallbacks(new AlarmControlCallback());
  // Créer un descripteur
  BLEDescriptor *alarmControlDesc = new BLEDescriptor(BLEUUID((uint16_t)0x2901));  // BLE2901 standard
  alarmControlDesc->setValue("Alarm Control");                                     // Description textuelle
  alarmControlChar->addDescriptor(alarmControlDesc);

  // Etat de l'alarme
  BLECharacteristic *alarmStateChar = service->createCharacteristic(
    ALARM_STATE_UUID, BLECharacteristic::PROPERTY_READ);
  alarmStateChar->setCallbacks(new AlarmStateCallback());
  // Créer un descripteur
  BLEDescriptor *alarmStateDesc = new BLEDescriptor(BLEUUID((uint16_t)0x2901));  // BLE2901 standard
  alarmStateDesc->setValue("Alarm State");                                       // Description textuelle
  alarmStateChar->addDescriptor(alarmStateDesc);

  // Desactiver l'alarme
  BLECharacteristic *alarmDisableChar = service->createCharacteristic(
    ALARM_DISABLE_UUID, BLECharacteristic::PROPERTY_WRITE);
  alarmDisableChar->setCallbacks(new AlarmDisableCallback());
  // Créer un descripteur
  BLEDescriptor *alarmDisableDesc = new BLEDescriptor(BLEUUID((uint16_t)0x2901));  // BLE2901 standard
  alarmDisableDesc->setValue("Alarm Disable");                                     // Description textuelle
  alarmDisableChar->addDescriptor(alarmDisableDesc);

  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);  // Rendre le service identifiable
  advertising->setScanResponse(true);         // Répondre aux scans BLE
  advertising->setMinPreferred(0x06);         // Optimisation pour appareils récents
  advertising->setMinPreferred(0x12);         // Compatibilité avec appareils modernes
  BLEDevice::startAdvertising();

}