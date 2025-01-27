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

#define MAX_BONDED_DEVICES 3
static uint32_t PASSKEY = 0;  // Sera généré aléatoirement

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
      activateAlarm();

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
    char alarmTime[20];  // Format "HH:MM,STATE"
    sprintf(alarmTime, "%02d:%02d,%d", myAlarm.hour, myAlarm.minute, myAlarm.set ? 1 : 0);
    pCharacteristic->setValue((uint8_t *)alarmTime, strlen(alarmTime));
    Serial.printf("Lecture de l'heure de l'alarme : %s (État: %s)\n", 
                 alarmTime, 
                 myAlarm.set ? "activée" : "désactivée");
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
        disableAlarm();  // Désactiver l'alarme

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
      if (myAlarm.triggered) {
        stopAlarm();
        stopAlarmSound();
      }
    } else {
      Serial.println("Commande invalide pour désactiver l'alarme");
    }
  }
};

// --- CALLBACKS DU SERVEUR --- //
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    Serial.println("Appareil connecté - En attente d'authentification");
    // Forcer la demande de sécurité immédiatement après la connexion
    uint16_t conn_id = server->getConnId();
    esp_bd_addr_t remote_bda;
    const uint8_t *bda = esp_bt_dev_get_address();
    memcpy(remote_bda, bda, sizeof(esp_bd_addr_t));
    esp_ble_set_encryption(remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
    BLEDevice::startAdvertising();
  }

  void onDisconnect(BLEServer *server) override {
    Serial.println("Appareil déconnecté.");
    server->startAdvertising();
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

// Nouvelle classe pour gérer la sécurité
class SecurityCallback : public BLESecurityCallbacks {
  uint32_t onPassKeyRequest() override {
    // Générer un code PIN aléatoire à 2 chiffres
    PASSKEY = random(10, 100);  // Génère un nombre entre 10 et 99
    Serial.printf("Nouveau code PIN généré : %02d\n", PASSKEY);

    return PASSKEY;
  }

  void onPassKeyNotify(uint32_t pass_key) override {
    Serial.printf("Code PIN à entrer : %06d\n", pass_key);
    // Afficher le code sur l'écran
    displayPasskey(String(pass_key));
    lastActiveTime = millis();  // Réinitialiser le timer
  }


  bool onConfirmPIN(uint32_t pin) override {
    Serial.printf("Confirmation du PIN : %06d\n", pin);
    return true;
  }

  bool onSecurityRequest() override {
    Serial.println("Demande de sécurité reçue");
    return true;
  }

  void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl) override {
    if (auth_cmpl.success) {
      Serial.println("Authentification réussie");
      clearPasskeyDisplay();  // Effacer le code PIN de l'écran

      // Vérifier le nombre d'appareils appairés
      int num_bonded = esp_ble_get_bond_device_num();
      if (num_bonded > MAX_BONDED_DEVICES) {
        // Supprimer le plus ancien appairage
        esp_ble_bond_dev_t *dev_list = new esp_ble_bond_dev_t[num_bonded];
        esp_ble_get_bond_device_list(&num_bonded, dev_list);
        esp_ble_remove_bond_device(dev_list[0].bd_addr);
        delete[] dev_list;
        Serial.println("Ancien appairage supprimé");
      }
    } else {
      Serial.println("Authentification échouée");
      clearPasskeyDisplay();  // Effacer le code PIN même en cas d'échec
    }
  }
};

void initBLE() {
  BLEDevice::init("AuroWake_1234");

  // Configuration de la sécurité
  BLESecurity *pSecurity = new BLESecurity();

  // Paramètres de sécurité
  esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;  // Requiert MITM protection
  esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;                     // Capacité d'afficher un code
  uint8_t key_size = 16;
  uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;

  // Configuration des paramètres de sécurité
  esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

  // Définir le niveau de cryptage
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  BLEDevice::setSecurityCallbacks(new SecurityCallback());

  // Configurer le serveur BLE
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  // Créer le service avec une taille suffisante
  BLEService *service = server->createService(BLEUUID(SERVICE_UUID), 30, 0);  // Augmenter le nombre de handles

  // Task Status (Notification)
  taskStatusChar = service->createCharacteristic(
    TASK_STATUS_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);
  BLE2902 *taskStatusDesc = new BLE2902();
  taskStatusDesc->setNotifications(true);
  taskStatusChar->addDescriptor(taskStatusDesc);

  // Current Time
  BLECharacteristic *currentTimeChar = service->createCharacteristic(
    CURRENT_TIME_UUID,
    BLECharacteristic::PROPERTY_WRITE);
  currentTimeChar->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM);
  currentTimeChar->setCallbacks(new CurrentTimeCallback());
  BLE2902 *currentTimeDesc = new BLE2902();
  currentTimeChar->addDescriptor(currentTimeDesc);
  currentTimeChar->addDescriptor(new BLEDescriptor(BLEUUID((uint16_t)0x2901)));

  // Alarm Time
  BLECharacteristic *alarmTimeChar = service->createCharacteristic(
    ALARM_TIME_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);
  alarmTimeChar->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM);
  alarmTimeChar->setCallbacks(new AlarmTimeCallback());
  BLE2902 *alarmTimeDesc = new BLE2902();
  alarmTimeChar->addDescriptor(alarmTimeDesc);
  alarmTimeChar->addDescriptor(new BLEDescriptor(BLEUUID((uint16_t)0x2901)));

  // Alarm Control
  BLECharacteristic *alarmControlChar = service->createCharacteristic(
    ALARM_CONTROL_UUID,
    BLECharacteristic::PROPERTY_WRITE);
  alarmControlChar->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM);
  alarmControlChar->setCallbacks(new AlarmControlCallback());
  BLE2902 *alarmControlDesc = new BLE2902();
  alarmControlChar->addDescriptor(alarmControlDesc);
  alarmControlChar->addDescriptor(new BLEDescriptor(BLEUUID((uint16_t)0x2901)));

  // Alarm Disable - Vérification
  BLECharacteristic *alarmDisableChar = service->createCharacteristic(
    ALARM_DISABLE_UUID,
    BLECharacteristic::PROPERTY_WRITE);
  alarmDisableChar->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM);
  alarmDisableChar->setCallbacks(new AlarmDisableCallback());
  BLE2902 *alarmDisableDesc = new BLE2902();
  alarmDisableChar->addDescriptor(alarmDisableDesc);

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