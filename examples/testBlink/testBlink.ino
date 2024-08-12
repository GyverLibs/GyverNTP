// пример выводит время каждую секунду
// а также два раза в секунду мигает светодиодом
// можно прошить на несколько плат - они будут мигать синхронно

#include <GyverNTP.h>

// список серверов, если "pool.ntp.org" не работает
//"ntp1.stratum2.ru"
//"ntp2.stratum2.ru"
//"ntp.msk-ix.ru"

void setup() {
    Serial.begin(115200);
    WiFi.begin("WIFI_SSID", "WIFI_PASS");
    while (WiFi.status() != WL_CONNECTED) delay(100);
    Serial.println("Connected");

    NTP.begin(3);  // запустить и указать часовой пояс

    // NTP.asyncMode(false);   // выключить асинхронный режим
    // NTP.ignorePing(true);   // не учитывать пинг до сервера
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    NTP.tick();

    // новая секунда!
    if (NTP.newSecond()) {
        Serial.println(NTP.toString());      // вывод даты и времени строкой
        Serial.println(NTP.dateToString());  // вывод даты строкой

        // можно сравнивать напрямую с unix
        if (NTP >= 12345) {
        }
        if (NTP == 123456) {
        }

        NTP.getUnix();  // unix секунды

        // парсинг unix на дату и время
        NTP.second();  // секунды
        NTP.minute();  // минуты и так далее

        // эффективнее использовать парсер Datime
        Datime dt(NTP);  // NTP само конвертируется в Datime

        dt.year;
        dt.month;
        dt.day;
        dt.hour;
        dt.minute;
        dt.second;
        dt.weekDay;
        dt.yearDay;
    }

    // блинкер
    if (NTP.ms() == 0) {
        delay(1);
        digitalWrite(LED_BUILTIN, 1);
    }

    if (NTP.ms() == 500) {
        delay(1);
        digitalWrite(LED_BUILTIN, 0);
    }
}