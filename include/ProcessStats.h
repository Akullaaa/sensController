#ifndef PROCESS_STATS_H
#define PROCESS_STATS_H

// Структура для статистики выполнения
struct ProcessStats {
  unsigned long callCount = 0;
  unsigned long totalTime = 0;
  unsigned long minTime = 0xFFFFFFFF; // Максимальное значение unsigned long
  unsigned long maxTime = 0;
  
  // Метод для обновления статистики
  void update(unsigned long executionTime) {
    callCount++;
    totalTime += executionTime;
    if (executionTime < minTime) minTime = executionTime;
    if (executionTime > maxTime) maxTime = executionTime;
  }
  
  // Метод для вычисления среднего времени
  unsigned long getAverageTime() const {
    return callCount > 0 ? totalTime / callCount : 0;
  }
  
  // Метод для формирования строки со статистикой
  String getStatsString() const {
    return "(" + String(callCount) + 
           ") " + String(minTime) + 
           "/" + String(maxTime) + 
           "/" + String(getAverageTime());
  }
};

#endif
