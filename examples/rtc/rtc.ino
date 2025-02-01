#include <Arduino.h>
#include <GyverNTP.h>

// GyverDS3231 поддерживает работу с GyverNTP
#include <GyverDS3231Min.h>
GyverDS3231Min rtc;

// можно написать свой класс и использовать любой другой RTC
class RTC : public VirtualRTC {
   public:
    void setUnix(uint32_t unix) {
        Serial.print("SET RTC: ");
        Serial.println(unix);
    }
    uint32_t getUnix() {
        return 1738015299ul;
    }
};
RTC vrtc;

void setup() {
    Serial.begin(115200);
    WiFi.begin("WIFI_SSID", "WIFI_PASS");
    // while (WiFi.status() != WL_CONNECTED) delay(100);
    Serial.println("Connected");

    // GyverDS3231
    Wire.begin();
    rtc.begin();

    NTP.begin(3);  // запустить и указать часовой пояс

    // подключить RTC
    // NTP.attachRTC(vrtc);
    NTP.attachRTC(rtc);
}

void loop() {
    if (NTP.tick()) {
        Serial.println(NTP.toString());
    }
}