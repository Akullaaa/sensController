#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <SPI.h>

#include "ProcessStats.h"
#include "Helpers.h"
#include "SDLogger.h"
#include "SoilSensor.h"
#include "Pump.h"
#include "PumpController.h"

// Инициализация LCD дисплея (адрес 0x27, 20 символов, 4 строки)
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Глобальная статистика для всех насосов
ProcessStats globalProcessStats;

// Глобальный объект логгера
SDLogger logger;

// Объявление контроллера насосов (определён ниже)
extern PumpController pumpController;

// Имя файла конфигурации на SD
const char* CONFIG_FILE = "config.txt";

// Прототипы функций конфигурации
void saveConfig();
void loadConfig();

// Функция для унифицированного логирования (Serial + SD)
void logMessage(const __FlashStringHelper* msg) {
  Serial.println(msg);
  logger.writeLog(msg);
}

void logMessage(const String& msg) {
  Serial.println(msg);
  // Для String конвертируем в константное сообщение для логгера
  if (logger.isInit()) {
    File f = SD.open("log.txt", FILE_WRITE);
    if (f) {
      unsigned long t = millis() / 1000;
      f.print('[');
      f.print(t / 3600);
      f.print(':');
      if ((t % 3600) / 60 < 10) f.print('0');
      f.print((t % 3600) / 60);
      f.print(':');
      if (t % 60 < 10) f.print('0');
      f.print(t % 60);
      f.print("] ");
      f.println(msg);
      f.close();
    }
  }
}

// Сохранение конфигурации (пороги и интервалы) на SD
void saveConfig() {
  SD.remove(CONFIG_FILE);
  File f = SD.open(CONFIG_FILE, FILE_WRITE);
  if (!f) {
    logMessage(F("ERR: CFG open W"));
    return;
  }
  for (int i = 0; i < pumpController.getPumpCount(); i++) {
    Pump* p = pumpController.getPumpAt(i);
    if (!p) continue;
    int soilPin = p->getSensor().getSoilPin();
    int threshold = p->getSensor().getDryThreshold();
    unsigned long interval = p->getSensor().getCheckInterval();
    f.print(F("TH_"));
    f.print(sensorPin(soilPin));
    f.print('=');
    f.println(threshold);
    f.print(F("IV_P"));
    f.print(p->getPumpPin());
    f.print('=');
    f.println(interval);
  }
  f.close();
  logMessage(F("CFG saved"));
}

// Загрузка конфигурации с SD
void loadConfig() {
  if (!SD.exists(CONFIG_FILE)) {
    logMessage(F("CFG not found"));
    return;
  }
  File f = SD.open(CONFIG_FILE, FILE_READ);
  if (!f) {
    logMessage(F("ERR: CFG open R"));
    return;
  }
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    int eq = line.indexOf('=');
    if (eq < 0) continue;
    String key = line.substring(0, eq);
    String val = line.substring(eq + 1);
    val.trim();
    if (key.startsWith("TH_")) {
      String pinStr = key.substring(3);
      int soilPin = parseSoilPin(pinStr);
      if (soilPin >= 0) {
        pumpController.setThresholdForSoilPin(soilPin, val.toInt());
      }
    } else if (key.startsWith("IV_P")) {
      String pinStr = key.substring(4);
      int pumpPin = pinStr.toInt();
      if (!(pumpPin <= 0 && pinStr != "0")) {
        pumpController.setIntervalForPumpPin(pumpPin, val.toInt());
      }
    }
  }
  f.close();
  logMessage(F("CFG loaded"));
}

// Создание контроллера насосов
PumpController pumpController(10); // Максимум 10 насосов

// Буфер для команд из Serial
String serialCmdBuffer;

void processCommand(const String& cmd) {
  String s = cmd;
  s.trim();
  if (s.length() == 0) return;

  auto printUsage = []() {
    logMessage(F("CMD: T <soil_pin> <threshold> (soil pin A0.. / 0..)") );
    logMessage(F("CMD: I P<pump_pin> <interval_ms> (pump pin digital, format P4)") );
  };

  if (s.startsWith("T") || s.startsWith("t")) {
    int sp1 = s.indexOf(' ');
    if (sp1 < 0) {
      printUsage();
      return;
    }
    String rest = s.substring(sp1 + 1);
    rest.trim();
    int sp2 = rest.indexOf(' ');
    if (sp2 < 0) {
      printUsage();
      return;
    }
    String pinTok = rest.substring(0, sp2);
    String valTok = rest.substring(sp2 + 1);
    pinTok.trim();
    valTok.trim();
    int soilPin = parseSoilPin(pinTok);
    if (soilPin < 0) {
      logMessage(F("ERR: bad pin"));
      return;
    }
    int threshold = valTok.toInt();
    pumpController.setThresholdForSoilPin(soilPin, threshold);
    saveConfig();
    return;
  }

  if (s.startsWith("I") || s.startsWith("i")) {
    int sp1 = s.indexOf(' ');
    if (sp1 < 0) {
      printUsage();
      return;
    }
    String rest = s.substring(sp1 + 1);
    rest.trim();
    int sp2 = rest.indexOf(' ');
    if (sp2 < 0) {
      printUsage();
      return;
    }
    String pinTok = rest.substring(0, sp2);
    String valTok = rest.substring(sp2 + 1);
    pinTok.trim();
    valTok.trim();
    if (pinTok.length() > 0 && (pinTok[0] == 'P' || pinTok[0] == 'p')) {
      pinTok = pinTok.substring(1);
      pinTok.trim();
    }
    int pumpPin = pinTok.toInt();
    if (pumpPin <= 0 && pinTok != "0") {
      logMessage(F("ERR: bad pump pin"));
      return;
    }
    unsigned long interval = valTok.toInt();
    if (interval == 0) {
      logMessage(F("ERR: interval must be > 0"));
      return;
    }
    pumpController.setIntervalForPumpPin(pumpPin, interval);
    saveConfig();
    return;
  }

  logMessage(F("ERR: unknown cmd"));
}

void handleSerialInput() {
  while (Serial.available()) {
    char c = Serial.read();
    Serial.print(c); // Эхо введённого символа
    if (c == '\n' || c == '\r') {
      if (serialCmdBuffer.length() > 0) {
        processCommand(serialCmdBuffer);
        serialCmdBuffer = "";
        Serial.println(); // Новая строка после ответа
      }
    } else {
      if (serialCmdBuffer.length() < 64) {
        serialCmdBuffer += c;
      } else {
        serialCmdBuffer = ""; // сбрасываем при переполнении
      }
    }
  }
}

void setup() {
  // Инициализация последовательного порта
  Serial.begin(BAUD_RATE);
  
  // Инициализация SD-карты для логирования
  logger.begin(SD_CS_PIN);
  
  // Добавление насосов в контроллер
  pumpController.addPump(SoilSensor(A1, 200, 44400), 4, 4000);  // Насос 1
  pumpController.addPump(SoilSensor(A3, 300, 55000), 5, 5000);   // Насос 2
  pumpController.addPump(SoilSensor(A2, 222, 66000), 6, 6000);  // Насос 3
  pumpController.addPump(SoilSensor(A0, 223, 77000), 7 , 7000);  // Насос 4

  // Загрузка конфигурации после регистрации насосов
  loadConfig();
  
  // Инициализация контроллера
  pumpController.setup();
}

void loop() {
  handleSerialInput();
  // Обработка всех насосов через контроллер
  pumpController.process();
}