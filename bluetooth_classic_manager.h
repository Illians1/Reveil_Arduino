#ifndef BLUETOOTH_CLASSIC_MANAGER_H
#define BLUETOOTH_CLASSICMANAGER_H

#include "AudioTools.h"
#include "BluetoothA2DPSink.h"
#include "esp_bt_device.h"
#include "display_manager.h"

void init_bluetooth();
void handle_bluetooth();
void avrc_metadata_callback(uint8_t id, const uint8_t *text);

extern unsigned long delai;

#endif