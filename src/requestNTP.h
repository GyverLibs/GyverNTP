#pragma once

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif
#include <WiFiUdp.h>

// получить unix время, 0 при ошибке
static uint32_t requestNTP(const char* host = "pool.ntp.org", uint16_t port = 123, uint16_t tout = 2000) {
    WiFiUDP udp;
    uint8_t buf[48] = {0b11100011};

    if (!udp.begin(1234) ||
        !udp.beginPacket(host, port) ||
        udp.write(buf, 48) != 48 ||
        !udp.endPacket()) {
        udp.stop();
        return 0;
    }

    while (udp.parsePacket() != 48) {
        if (!--tout) {
            udp.stop();
            return 0;
        }
        delay(1);
    }

    if (udp.remotePort() != port ||
        udp.read(buf, 48) != 48 ||
        !buf[40]) {
        udp.stop();
        return 0;
    }

    return (((uint32_t)buf[40] << 24) | ((uint32_t)buf[41] << 16) | ((uint32_t)buf[42] << 8) | buf[43]) - 2208988800ul;
}
