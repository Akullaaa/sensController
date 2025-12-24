#ifndef SOIL_SENSOR_H
#define SOIL_SENSOR_H

// Класс для датчика сухости
class SoilSensor {
private:
  int soilPin;              // Пин датчика сухости
  int dryThreshold;         // Порог сухости
  unsigned long checkInterval; // Интервал проверки
  unsigned long lastCheckTime; // Время последней проверки
  int lastMoistureValue;    // Последнее измеренное значение сухости

public:
  // Конструктор
  SoilSensor(int soilPin, int dryThreshold, unsigned long checkInterval)
    : soilPin(soilPin), dryThreshold(dryThreshold), checkInterval(checkInterval), lastCheckTime(0), lastMoistureValue(0) {
      // Инициализируем lastMoistureValue сразу при создании объекта
      pinMode(soilPin, INPUT); // Важно настроить пин как вход
      lastMoistureValue = readMoisture();
    }

  // Метод для проверки, пришло ли время для следующей проверки
  bool isCheckTime() {
    return millis() - lastCheckTime >= checkInterval;
  }

  // Метод для чтения сухости
  int readMoisture() {
    // Обновляем время последней проверки
    lastCheckTime = millis();
    lastMoistureValue = analogRead(soilPin); // Сохраняем последнее значение
    return lastMoistureValue;
  }

  // Метод для проверки необходимости полива
  bool needsWatering(int moisture) {
    return moisture >= dryThreshold;
  }

  // Установка порога сухости
  void setDryThreshold(int threshold) {
    dryThreshold = threshold;
  }

  // Установка интервала проверки в миллисекундах
  void setCheckInterval(unsigned long interval) {
    checkInterval = interval;
  }

  // Метод для получения оставшегося времени до следующей проверки в секундах
  unsigned long getSecondsUntilNextCheck() const {
    unsigned long elapsed = millis() - lastCheckTime;
    if (elapsed >= checkInterval) {
      return 0; // Проверка должна произойти прямо сейчас
    }
    return (checkInterval - elapsed) / 1000;
  }

  // Геттеры
  int getSoilPin() const { return soilPin; }
  int getDryThreshold() const { return dryThreshold; }
  unsigned long getCheckInterval() const { return checkInterval; }
  unsigned long getLastCheckTime() const { return lastCheckTime; }
  int getLastMoistureValue() const { return lastMoistureValue; }
};

#endif
