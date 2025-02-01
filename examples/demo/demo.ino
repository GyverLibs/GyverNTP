#include <Arduino.h>
#include <GyverNTP.h>

void setup() {
    Serial.begin(115200);
    WiFi.begin("WIFI_SSID", "WIFI_PASS");
    while (WiFi.status() != WL_CONNECTED) delay(100);
    Serial.println("Connected");

    // обработчик ошибок
    NTP.onError([]() {
        Serial.println(NTP.readError());
        Serial.print("online: ");
        Serial.println(NTP.online());
    });

    // обработчик секунды (вызывается из тикера)
    NTP.onSecond([]() {
        Serial.println("new second!");
    });

    // обработчик синхронизации (вызывается из sync)
    // NTP.onSync([](uint32_t unix) {
    //     Serial.println("sync: ");
    //     Serial.print(unix);
    // });

    NTP.begin(3);                           // запустить и указать часовой пояс
    // NTP.setPeriod(30);                   // период синхронизации в секундах
    // NTP.setHost("ntp1.stratum2.ru");     // установить другой хост
    // NTP.setHost(IPAddress(1, 2, 3, 4));  // установить другой хост
    // NTP.asyncMode(false);                // выключить асинхронный режим
    // NTP.ignorePing(true);                // не учитывать пинг до сервера
    // NTP.updateNow();                     // обновить прямо сейчас
}

void loop() {
    // тикер вернёт true каждую секунду в 0 мс секунды, если время синхронизировано
    if (NTP.tick()) {
        // вывод даты и времени строкой
        Serial.print(NTP.toString());  // NTP.timeToString(), NTP.dateToString()
        Serial.print(':');
        Serial.println(NTP.ms());  // + миллисекунды текущей секунды. Внутри tick всегда равно 0

        // вывод в Datime
        Datime dt = NTP;  // или Datime dt(NTP)
        dt.year;
        dt.second;
        dt.hour;
        dt.weekDay;
        dt.yearDay;
        // ... и прочие методы и переменные Datime

        // чтение напрямую, медленнее чем вывод в Datime
        NTP.second();
        NTP.minute();
        NTP.year();
        // ... и прочие методы StampConvert

        // сравнение
        NTP == DaySeconds(12, 35, 0);            // сравнение с DaySeconds (время равно 12:35:00)
        NTP == 1738237474;                       // сравнение с unix
        NTP == Datime(2025, 1, 30, 14, 14, 30);  // сравнение с Datime
    }

    if (NTP.newSecond()) {
        // новую секунду можно поймать и здесь
    }

    // изменился онлайн-статус
    if (NTP.statusChanged()) {
        Serial.print("STATUS: ");
        Serial.println(NTP.online());
    }
}