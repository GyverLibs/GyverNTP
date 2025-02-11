#pragma once

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif
#include <WiFiUdp.h>

static uint32_t requestNTP(const char* host = "pool.ntp.org", uint16_t port = 123, uint16_t tout = 2000) {
    WiFiUDP udp;
    if (!udp.begin(1234)) return 0;
    if (!udp.beginPacket(host, port)) return 0;

    uint8_t buf[48] = {0b11100011};
    if (udp.write(buf, 48) != 48) return 0;
    if (!udp.endPacket()) return 0;

    while (udp.parsePacket() != 48) {
        if (!--tout) return 0;
        delay(1);
    }
    if (udp.remotePort() != port) return 0;
    if (udp.read(buf, 48) != 48) return 0;
    if (!buf[40]) return 0;
    return (((buf[40] << 8 | buf[41]) << 16) | (buf[42] << 8 | buf[43])) - 2208988800ul;
}