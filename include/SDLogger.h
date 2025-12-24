#ifndef SDLOGGER_H
#define SDLOGGER_H

#include <SD.h>
#include "Helpers.h"

// Пин CS для SD-карты (обычно 10 для Arduino Nano)
#define SD_CS_PIN 10

// Класс для логирования на SD-карту (оптимизирован для экономии RAM)
class SDLogger {
private:
  bool initialized;

public:
  SDLogger() : initialized(false) {}

  bool begin(int csPin = SD_CS_PIN) {
    Serial.print(F("Init SD..."));
    if (!SD.begin(csPin)) {
      Serial.println(F(" ERR!"));
      initialized = false;
      return false;
    }
    Serial.println(F(" OK"));
    initialized = true;
    writeLog(F("=START="));
    return true;
  }

  bool isInit() const { return initialized; }

  void writeLog(const __FlashStringHelper* msg) {
    if (!initialized) return;
    File f = SD.open("log.txt", FILE_WRITE);
    if (!f) return;
    
    unsigned long t = millis() / 1000;
    f.print('[');
    f.print(t / 3600);
    f.print(':');
    if ((t % 3600) / 60 < 10) f.print('0');
    f.print((t % 3600) / 60);
    f.print(':');
    if (t % 60 < 10) f.print('0');
    f.print(t % 60);
    f.print(F("] "));
    f.println(msg);
    f.close();
  }
  
  void logPump(int pin, int sensor, int moisture, bool on) {
    if (!initialized) return;
    File f = SD.open("log.txt", FILE_WRITE);
    if (!f) return;
    
    unsigned long t = millis() / 1000;
    f.print('[');
    f.print(t / 3600);
    f.print(':');
    if ((t % 3600) / 60 < 10) f.print('0');
    f.print((t % 3600) / 60);
    f.print(':');
    if (t % 60 < 10) f.print('0');
    f.print(t % 60);
    f.print(F("] P"));
    f.print(pin);
    f.print(on ? F(" ON ") : F(" OFF "));
    f.print(sensorPin(sensor));
    f.print(F(" M="));
    f.println(moisture);
    f.close();
  }
};

#endif
