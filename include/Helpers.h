#ifndef HELPERS_H
#define HELPERS_H

// Функция для получения свободной оперативной памяти
int getFreeMemory() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

// Функция для форматирования аналогового входа
String sensorPin(int pin) {
  if (pin >= A0) {
    return String("A") + String(pin - A0);
  }
  return String(pin);
}

// Парсер пина из строки вида "A1" или "1" -> числовой пин
int parseSoilPin(const String& token) {
  if (token.length() == 0) return -1;
  if (token.charAt(0) == 'A' || token.charAt(0) == 'a') {
    int num = token.substring(1).toInt();
    return A0 + num;
  }
  return token.toInt();
}

#endif
