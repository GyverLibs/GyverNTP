[![Foo](https://img.shields.io/badge/Version-1.0-brightgreen.svg?style=flat-square)](#versions)
[![Foo](https://img.shields.io/badge/Website-AlexGyver.ru-blue.svg?style=flat-square)](https://alexgyver.ru/)
[![Foo](https://img.shields.io/badge/%E2%82%BD$%E2%82%AC%20%D0%9D%D0%B0%20%D0%BF%D0%B8%D0%B2%D0%BE-%D1%81%20%D1%80%D1%8B%D0%B1%D0%BA%D0%BE%D0%B9-orange.svg?style=flat-square)](https://alexgyver.ru/support_alex/)

# GyverNTP
Библиотека для получения точного времени с NTP сервера для esp8266/esp32
- Работает на стандартной библиотеке WiFiUdp.h
- Учёт времени ответа сервера и задержки соединения
- Получение времени с точностью до нескольких миллисекунд
- Получение UNIX-времени, а также миллисекунд, секунд, минут, часов, дня, месяца, года и дня недели
- Синхронизация по таймеру
- Обработка ошибок

### Совместимость
esp8266, esp32

## Содержание
- [Установка](#install)
- [Инициализация](#init)
- [Использование](#usage)
- [Пример](#example)
- [Версии](#versions)
- [Баги и обратная связь](#feedback)

<a id="install"></a>
## Установка
- Библиотеку можно найти по названию **GyverNTP** и установить через менеджер библиотек в:
    - Arduino IDE
    - Arduino IDE v2
    - PlatformIO
- [Скачать библиотеку](https://github.com/GyverLibs/GyverNTP/archive/refs/heads/main.zip) .zip архивом для ручной установки:
    - Распаковать и положить в *C:\Program Files (x86)\Arduino\libraries* (Windows x64)
    - Распаковать и положить в *C:\Program Files\Arduino\libraries* (Windows x32)
    - Распаковать и положить в *Документы/Arduino/libraries/*
    - (Arduino IDE) автоматическая установка из .zip: *Скетч/Подключить библиотеку/Добавить .ZIP библиотеку…* и указать скачанный архив
- Читай более подробную инструкцию по установке библиотек [здесь](https://alexgyver.ru/arduino-first/#%D0%A3%D1%81%D1%82%D0%B0%D0%BD%D0%BE%D0%B2%D0%BA%D0%B0_%D0%B1%D0%B8%D0%B1%D0%BB%D0%B8%D0%BE%D1%82%D0%B5%D0%BA)

<a id="init"></a>
## Инициализация
```cpp
GyverNTP ntp;               // параметры по умолчанию (gmt 0, период 1 минута)
GyverNTP(gmt);              // часовой пояс (например Москва 3)
GyverNTP(gmt, period);      // часовой пояс и период обновления в секундах
```

<a id="usage"></a>
## Использование
```cpp
void setGMT(int8_t gmt);        // установить часовой пояс
void setPeriod(uint16_t prd);   // установить период обновления в секундах
void setHost(char* host);       // установить хост (по умолч. "pool.ntp.org")
void begin();                   // запустить
void end();                     // остановить

uint8_t tick();                 // тикер, обновляет время по своему таймеру. Вернёт статус (см. ниже)
uint8_t requestTime();          // вручную запросить и обновить время с сервера. Вернёт статус (см. ниже)
uint32_t msFromUpdate();        // миллисекунд с последнего обновления (с 0 секунд unix)
uint32_t unix();                // unix время

uint16_t ms();                  // миллисекунды текущей секунды
uint8_t second();               // получить секунды
uint8_t minute();               // получить минуты
uint8_t hour();                 // получить часы
uint8_t day();                  // получить день месяца
uint8_t month();                // получить месяц
uint16_t year();                // получить год
uint8_t dayWeek();              // получить день недели

String timeString();            // получить строку времени формата ЧЧ:ММ:СС
String dateString();            // получить строку даты формата ДД.ММ.ГГГГ

int16_t ping();                 // получить пинг сервера
uint8_t status();               // получить статус системы

// 0 - всё ок
// 1 - не запущен UDP
// 2 - не подключен WiFi
// 3 - ошибка подключения к серверу
// 4 - ошибка отправки пакета
// 5 - таймаут ответа сервера
// 6 - получен некорректный ответ сервера
```

### Особенности
- Нужно вызывать `tick()` в главном цикле программы `loop()`, он синхронизирует время по своему таймеру
- Библиотека продолжает считать время даже после пропадания синхронизации

<a id="example"></a>
## Пример
```cpp
// пример выводит время каждую секунду
// а также два раза в секунду мигает светодиодом
// можно прошить на несколько плат - они будут мигать синхронно

#include <ESP8266WiFi.h>  // esp8266
//#include <WiFi.h>       // esp32

#include <GyverNTP.h>
GyverNTP ntp(3);

// список серверов, если "pool.ntp.org" не работает
//"ntp1.stratum2.ru"
//"ntp2.stratum2.ru"
//"ntp.msk-ix.ru"

void setup() {
  Serial.begin(115200);
  WiFi.begin("WIFI_SSID", "WIFI_PASS");
  while (WiFi.status() != WL_CONNECTED) delay(100);
  Serial.println("Connected");

  ntp.begin();
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  ntp.tick();
  
  if (ntp.ms() == 0) {
    delay(1);
    digitalWrite(LED_BUILTIN, 1);
  }
  if (ntp.ms() == 500) {
    delay(1);
    digitalWrite(LED_BUILTIN, 0);
    Serial.println(ntp.timeString());
    Serial.println(ntp.dateString());
    Serial.println();
  }
}
```

<a id="versions"></a>
## Версии
- v1.0

<a id="feedback"></a>
## Баги и обратная связь
При нахождении багов создавайте **Issue**, а лучше сразу пишите на почту [alex@alexgyver.ru](mailto:alex@alexgyver.ru)  
Библиотека открыта для доработки и ваших **Pull Request**'ов!
