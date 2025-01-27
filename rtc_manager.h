#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include <RTClib.h>

// Forward declaration
void stopAlarmSound();

// Structure pour les alarmes
struct AlarmData {
  int hour;
  int minute;
  bool set;
  bool triggered;
  unsigned long startTime;
};

extern RTC_DS3231 rtc;          // Objet RTC partagé
extern AlarmData myAlarm;       // Variable pour l'alarme
extern DateTime lastAlarmTime;  // Dernière sonnerie de l'alarme
extern bool timeSet;            // Indique si l'heure est réglée

const unsigned long ALARM_AUTO_STOP_DURATION = 600000; // 10 minutes

void initRTC();
void updateAlarm(const DateTime &now);
void stopAlarm();
void setRTC(const DateTime &time);
void disableAlarm();
void activateAlarm();

#endif