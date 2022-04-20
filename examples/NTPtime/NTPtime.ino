#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include <TimeLib.h> //http://playground.arduino.cc/Code/Time/
int TimeZone = 3;
const PROGMEM char* NTPServerName = "pool.ntp.org";
#define NTPport 123
WiFiUDP NTP_UDP;
uint32_t UPDATETime;
uint32_t StartMlTime;
bool NTPUpdate;

void setup() {
  Serial.begin(9600);

  WiFi.mode(WIFI_STA);
  WiFi.begin("ssid", "pass");
  NTP_UDP.begin(NTPport);
}

void loop() {
  NTPLoop();
}
