#ifndef PUMP_H
#define PUMP_H

#include "SoilSensor.h"
#include "SDLogger.h"
#include "ProcessStats.h"
#include "Helpers.h"

extern ProcessStats globalProcessStats;
extern SDLogger logger;

// Объявление функции логирования (определена в main.cpp)
void logMessage(const __FlashStringHelper* msg);
void logMessage(const String& msg);

// Класс для управления насосом
class Pump {
private:
  SoilSensor sensor;        // Объект датчика (копия)
  int pumpPin;              // Пин реле насоса
  unsigned long pumpDuration; // Длительность работы насоса
  bool isPumping;           // Флаг активности насоса
  unsigned long pumpStartTime; // Время начала работы насоса
  unsigned long lastPumpOffTime;

public:
  // Конструктор принимает временный объект SoilSensor
  Pump(SoilSensor sensor, int pumpPin, unsigned long pumpDuration)
    : sensor(sensor), pumpPin(pumpPin), pumpDuration(pumpDuration),
      isPumping(false), pumpStartTime(0), lastPumpOffTime(0) {}

  // Метод для настройки пина насоса
  void setupPin() {
    pinMode(pumpPin, OUTPUT);
  }

  // Метод для проверки, истекло ли время работы насоса
  bool isPumpTimeExpired() {
    return millis() - pumpStartTime >= pumpDuration;
  }

  // Метод для включения насоса
  void startPumping() {
    digitalWrite(pumpPin, LOW);
    isPumping = true;
    pumpStartTime = millis();
    String msg = String(F("Pump ")) + pumpPin + F(" ON s=") + getSensor().getSoilPin();
    logMessage(msg);
    logger.logPump(pumpPin, getSensor().getSoilPin(), getSensor().getLastMoistureValue(), true);
  }

  // Метод для выключения насоса
  void stopPumping() {
    digitalWrite(pumpPin, HIGH);
    isPumping = false;
    unsigned long workTime = millis() - pumpStartTime;
    String msg = String(F("Pump ")) + pumpPin + F(" OFF t=") + (workTime / 1000) + F("s");
    logMessage(msg);
    logger.logPump(pumpPin, getSensor().getSoilPin(), getSensor().getLastMoistureValue(), false);
  }

  // Основной метод обработки насоса
  void process() {
    unsigned long startTime = micros(); // Засекаем время начала
    
    // Проверяем, не активен ли насос в данный момент
    if (isPumping) {
      // Если насос работает, проверяем, не истекло ли время работы
      if (isPumpTimeExpired()) {
        stopPumping();
      }
      
      unsigned long endTime = micros(); // Засекаем время окончания
      globalProcessStats.update(endTime - startTime); // Обновляем статистику
      return; // Прерываем выполнение, если насос активен
    }
    
    // Проверяем, пришло ли время для следующей проверки датчика
    if (sensor.isCheckTime()) {
      int moisture = sensor.readMoisture();
      
      // Выводим информацию в монитор порта и логгер
      String msg = sensorPin(sensor.getSoilPin()) + 
                   F(" ") + moisture + F("(") + sensor.getDryThreshold() + 
                   F(") RAM=") + getFreeMemory();
      logMessage(msg);

      // Проверяем, слишком ли сухая почва
      if (sensor.needsWatering(moisture)) {
        startPumping();
      }
    }
    
    unsigned long endTime = micros(); // Засекаем время окончания
    globalProcessStats.update(endTime - startTime); // Обновляем статистику
  }

  // Геттеры
  bool getIsPumping() const { return isPumping; }
  unsigned long getPumpStartTime() const { return pumpStartTime; }
  unsigned long getPumpDuration() const { return pumpDuration; }
  SoilSensor getSensor() const { return sensor; }
  void setDryThreshold(int threshold) { sensor.setDryThreshold(threshold); }
  void setCheckInterval(unsigned long interval) { sensor.setCheckInterval(interval); }
  int getPumpPin() const { return pumpPin; }
  unsigned long getLastPumpOffTime() const { return lastPumpOffTime; }
};

#endif
