// пример использования другого UDP клиента

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <GyverNTPClient.h>
#include <WiFiUdp.h>

WiFiUDP udp;
GyverNTPClient ntp(udp);

void setup() {
    Serial.begin(115200);
    WiFi.begin("AlexMain", "lolpass12345");
    while (WiFi.status() != WL_CONNECTED) delay(100);
    Serial.println("Connected");

    ntp.begin(3);
}

void loop() {
    if (ntp.tick()) {
        Serial.println(ntp.toString());
    }
}