#include "rtc_manager.h"

RTC_DS3231 rtc;
AlarmData myAlarm = { 0, 0, false, false, 0 };
DateTime lastAlarmTime(0, 0, 0, 0, 0, 0);
bool timeSet = false;

void initRTC() {
  if (!rtc.begin()) {
    Serial.println("Erreur: Impossible de détecter le RTC");
    while (1)
      ;
  }
}

void updateAlarm(const DateTime &now) {
  if (myAlarm.set && !myAlarm.triggered && now.hour() == myAlarm.hour && now.minute() == myAlarm.minute
      && (now.day() != lastAlarmTime.day() || now.hour() != lastAlarmTime.hour() || now.minute() != lastAlarmTime.minute())
      && timeSet) {
    myAlarm.triggered = true;
    myAlarm.startTime = millis();
    lastAlarmTime = now;
  }

  // Vérifier si l'alarme doit être arrêtée automatiquement
  if (myAlarm.triggered && (millis() - myAlarm.startTime >= ALARM_AUTO_STOP_DURATION)) {
    stopAlarm();
    stopAlarmSound();
    disableAlarm();
    Serial.println("Alarme arrêtée et désactivée automatiquement après 10 secondes");
  }
}

void stopAlarm() {
  myAlarm.triggered = false;
  Serial.println("Alarme arrêtée.");
}

void disableAlarm() {
  myAlarm.set = false;
  Serial.println("Alarme désactivée.");
}

void activateAlarm() {
  myAlarm.set = true;
  Serial.println("Alarme activée.");
}

void setRTC(const DateTime &time) {
  rtc.adjust(time);
}