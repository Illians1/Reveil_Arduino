#include "rtc_manager.h"

RTC_DS3231 rtc;
AlarmData myAlarm = {0, 0, false, false, 0};
DateTime lastAlarmTime(0, 0, 0, 0, 0, 0);
bool timeSet = false;

void initRTC() {
  if (!rtc.begin()) {
    Serial.println("Erreur: Impossible de détecter le RTC");
    while (1);
  }
}

void updateAlarm(const DateTime &now) {
  if (myAlarm.set && !myAlarm.triggered &&
      now.hour() == myAlarm.hour && now.minute() == myAlarm.minute
      && (now.day() != lastAlarmTime.day() || now.hour() != lastAlarmTime.hour() || now.minute() != lastAlarmTime.minute()) 
      && timeSet) {
    myAlarm.triggered = true;
    myAlarm.startTime = millis();
    lastAlarmTime = now;
  }
}

bool shouldTriggerAlarm(const DateTime &now) {
  return myAlarm.triggered && millis() - myAlarm.startTime > 10 * 60 * 1000;
}

void stopAlarm() {
  myAlarm.triggered = false;
  Serial.println("Alarme arrêtée.");
}

void setRTC(const DateTime &time) {
  rtc.adjust(time);
}