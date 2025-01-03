#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLESecurity.h>

void initBLE();

void handleAlarmNotifications();

extern unsigned long lastNotificationTime;  // Temps du dernier envoi de notification

#endif