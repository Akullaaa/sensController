#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "Pump.h"
#include "Helpers.h"

extern LiquidCrystal_I2C lcd;

// Объявление функции логирования (определена в main.cpp)
void logMessage(const __FlashStringHelper* msg);
void logMessage(const String& msg);

// Класс контроллера насосов
class PumpController {
private:
  Pump** pumps;           // Массив указателей на насосы
  int pumpCount;          // Количество насосов
  int maxPumps;           // Максимальное количество насосов
  unsigned long lastLcdUpdate; // Время последнего обновления LCD
  unsigned long lcdUpdateInterval; // Интервал обновления LCD (мс)
  String lastLines[4];    // Последние показанные строки на LCD
  String lastSeconds[4];  // Последние показанные секунды ожидания

public:
  // Конструктор
  PumpController(int maxPumps = 10, unsigned long lcdUpdateInterval = 1000) 
    : pumpCount(0), maxPumps(maxPumps), lastLcdUpdate(0), lcdUpdateInterval(lcdUpdateInterval) {
    pumps = new Pump*[maxPumps];
    for (int i = 0; i < maxPumps; i++) {
      pumps[i] = nullptr;
    }
    for (int i = 0; i < 4; i++) {
      lastLines[i] = "";
      lastSeconds[i] = "";
    }
  }

  // Деструктор для очистки памяти
  ~PumpController() {
    for (int i = 0; i < pumpCount; i++) {
      delete pumps[i];
    }
    delete[] pumps;
  }

  // Метод для добавления насоса
  bool addPump(SoilSensor sensor, int pumpPin, unsigned long pumpDuration) {
    if (pumpCount >= maxPumps) {
      logMessage(F("ERR: Max pumps"));
      return false;
    }
    
    pumps[pumpCount] = new Pump(sensor, pumpPin, pumpDuration);
    pumpCount++;
    return true;
  }

  // Метод инициализации для раздела setup
  void setup() {
    // Инициализация LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Init..."));
    
    for (int i = 0; i < pumpCount; i++) {
      pumps[i]->setupPin();
      pumps[i]->stopPumping();
    }
    Serial.print(F("System ready. Pumps="));
    Serial.print(pumpCount);
    Serial.print(F(" RAM="));
    Serial.println(getFreeMemory());
    
    logMessage(F("INIT_OK"));
    
    delay(2000);
    lcd.clear();
  }

  // Метод обновления LCD дисплея
  void updateLCD() {
    unsigned long now = millis();
    bool intervalReady = (now - lastLcdUpdate >= lcdUpdateInterval);
    if (intervalReady) {
      lastLcdUpdate = now;
    }
    
    // Отображаем данные для каждого насоса (максимум 4), обновляя только изменившиеся строки
    for (int i = 0; i < 4; i++) {
      lcd.setCursor(0, i);
      
      // Формат: "S1:123 T:200 ON  :P4" или с секундами ожидания
      String line;
      String secondsStr;
      if (i < pumpCount) {
        int moisture = pumps[i]->getSensor().getLastMoistureValue();
        int threshold = pumps[i]->getSensor().getDryThreshold();
        bool pumping = pumps[i]->getIsPumping();
        int pumpPin = pumps[i]->getPumpPin();
        int soilPin = pumps[i]->getSensor().getSoilPin();

        auto format3 = [](int value) {
          String s;
          if (value < 100) s += "0";
          if (value < 10) s += "0";
          s += String(value);
          return s;
        };

        auto formatSec5 = [](long value) {
          String s = String(value);
          while (s.length() < 5) s = " " + s; // Выравниваем вправо
          if (s.length() > 5) {
            s = s.substring(s.length() - 5);
          }
          return s;
        };
        
        line = sensorPin(soilPin) + ":";
        line += format3(moisture);
        line += "(" + format3(threshold) + ") ";
        
        if (pumping) {
          long remainingMs = (long)pumps[i]->getPumpDuration() - (long)(millis() - pumps[i]->getPumpStartTime());
          long remainingSec = remainingMs / 1000;
          if (remainingSec < 0) remainingSec = 0;
          secondsStr = formatSec5(-remainingSec); // Показываем со знаком минус
          line += secondsStr;
        } else {
          unsigned long secondsLeft = pumps[i]->getSensor().getSecondsUntilNextCheck();
          secondsStr = formatSec5((long)secondsLeft);
          line += secondsStr;
        }
        
        line += ":";
        line += (pumping ? "O" : "P");
        line += pumpPin;
      } else {
        line = ""; // Пустая строка, если насос отсутствует
      }

      // Паддинг до 20 символов для стирания хвостов старого текста
      if (line.length() < 20) {
        line += String(' ', 20 - line.length());
      } else if (line.length() > 20) {
        line = line.substring(0, 20);
      }

      bool secondsChanged = (secondsStr != lastSeconds[i]);
      bool lineChanged = (line != lastLines[i]);

      if (intervalReady || secondsChanged || lineChanged) {
        lcd.print(line);
        lastLines[i] = line;
        lastSeconds[i] = secondsStr;
      }
    }
  }

  // Метод process для loop
  void process() {
    for (int i = 0; i < pumpCount; i++) {
      pumps[i]->process();
    }
    // Обновляем LCD дисплей
    updateLCD();
  }

  bool setThresholdForSoilPin(int soilPin, int threshold) {
    for (int i = 0; i < pumpCount; i++) {
      if (pumps[i]->getSensor().getSoilPin() == soilPin) {
        pumps[i]->setDryThreshold(threshold);
        String msg = String(F("TH set ")) + sensorPin(soilPin) + F(" -> ") + threshold;
        logMessage(msg);
        return true;
      }
    }
    logMessage(String(F("ERR: soil pin not found ")) + soilPin);
    return false;
  }

  bool setIntervalForPumpPin(int pumpPin, unsigned long interval) {
    for (int i = 0; i < pumpCount; i++) {
      if (pumps[i]->getPumpPin() == pumpPin) {
        pumps[i]->setCheckInterval(interval);
        String msg = String(F("IV set P")) + pumpPin + F(" -> ") + interval + F("ms");
        logMessage(msg);
        return true;
      }
    }
    logMessage(String(F("ERR: pump pin not found ")) + pumpPin);
    return false;
  }

  // Геттеры
  int getPumpCount() const { return pumpCount; }
  int getMaxPumps() const { return maxPumps; }
  Pump* getPumpAt(int idx) const {
    if (idx < 0 || idx >= pumpCount) return nullptr;
    return pumps[idx];
  }
};

#endif
