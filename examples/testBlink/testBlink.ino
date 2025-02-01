// пример раз в секунду мигает светодиодом
// можно прошить на несколько плат - они будут мигать синхронно

#include <Arduino.h>
#include <GyverNTP.h>

void setup() {
    Serial.begin(115200);
    WiFi.begin("WIFI_SSID", "WIFI_PASS");
    while (WiFi.status() != WL_CONNECTED) delay(100);
    Serial.println("Connected");

    NTP.begin(3);  // запустить и указать часовой пояс

    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    NTP.tick();

    if (NTP.ms() == 0) {
        delay(1);
        digitalWrite(LED_BUILTIN, 1);
    }

    if (NTP.ms() == 500) {
        delay(1);
        digitalWrite(LED_BUILTIN, 0);
    }
}