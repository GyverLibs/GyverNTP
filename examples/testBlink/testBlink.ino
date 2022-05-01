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
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  WiFi.begin("WIFI_SSID", "WIFI_PASS");
  while (WiFi.status() != WL_CONNECTED) delay(100);
  Serial.println("Connected");

  ntp.begin();
  //ntp.asyncMode(false);   // выключить асинхронный режим
  //ntp.ignorePing(true);   // не учитывать пинг до сервера
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
