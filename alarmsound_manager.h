#ifndef ALARMSOUND_MANAGER_H
#define ALARMSOUND_MANAGER_H

#include "bluetooth_classic_manager.h"
#include "rtc_manager.h"

#include <SPIFFS.h>
#include "AudioTools/AudioLibs/AudioSourceSPIFFS.h"
#include "AudioTools/AudioCodecs/CodecWAV.h"

extern I2SStream i2s;
extern BluetoothA2DPSink a2dp_sink;

void handleAlarmSound();
void playAlarmSound();
void stopAlarmSound();

#endif