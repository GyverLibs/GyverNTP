#pragma once

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#else
#error "No implementation for given platform"
#endif

#include <WiFiUdp.h>

#include "GyverNTPClient.h"

class GyverNTP : public GyverNTPClient {
   public:
    GyverNTP(int16_t gmt = 0, uint16_t prd = 3600) : GyverNTPClient(udp, gmt, prd) {}

   private:
    WiFiUDP udp;
};

extern GyverNTP NTP;