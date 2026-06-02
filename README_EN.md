This is an automatic translation and may be incorrect in some places. See the source README and examples for authoritative information.

[![latest](https://img.shields.io/github/v/release/GyverLibs/GyverNTP.svg?color=brightgreen)](https://github.com/GyverLibs/GyverNTP/releases/latest/download/GyverNTP.zip)
[![PIO](https://badges.registry.platformio.org/packages/gyverlibs/library/GyverNTP.svg)](https://registry.platformio.org/libraries/gyverlibs/GyverNTP)
[![Foo](https://img.shields.io/badge/Website-AlexGyver.ru-blue.svg?style=flat-square)](https://alexgyver.ru/)
[![Foo](https://img.shields.io/badge/%E2%82%BD%24%E2%82%AC%20%D0%9F%D0%BE%D0%B4%D0%B4%D0%B5%D1%80%D0%B6%D0%B0%D1%82%D1%8C-%D0%B0%D0%B2%D1%82%D0%BE%D1%80%D0%B0-orange.svg?style=flat-square)](https://alexgyver.ru/support_alex/)
[![Foo](https://img.shields.io/badge/README-ENGLISH-blueviolet.svg?style=flat-square)](https://github-com.translate.goog/GyverLibs/GyverNTP?_x_tr_sl=ru&_x_tr_tl=en)  

[![Foo](https://img.shields.io/badge/ПОДПИСАТЬСЯ-НА%20ОБНОВЛЕНИЯ-brightgreen.svg?style=social&logo=telegram&color=blue)](https://t.me/GyverLibs)

# GyverNTP
Library to get accurate time from NTP server for esp8266/esp32
- It works on the standard Udp interface. h h
- Accounting for server response time and connection delay
- Getting time to within milliseconds
- Integration with the library[Stamp](https://github.com/GyverLibs/Stamp)Unix unpacking in hours, minutes, etc.
- Automatic synchronization
- Maintenance of time on the basis of millis() between synchronizations
- Secondary timer for convenience of automation
- Error handling
- Asynchronous mode
- Support for external RTC
- DNS caching, stable operation and the ability to verify the availability of the Internet

### Compatibility
All platforms.

### Dependencies
- [Stamp](https://github.com/GyverLibs/Stamp) v1.5.0+

## Contents
- [Initialization](#init)
- [Use of use](#usage)
- [Example](#example)
- [Versions](#versions)
- [Installation](#install)
- [Bugs and feedback](#feedback)

<a id="init"></a>

## Initialization
```cpp
GyverNTP;                 // default parameters (gmt 0, period 3600 seconds (1 hour))
GyverNTP(gmt);            // Time zone in hours (for example, Moscow 3)
GyverNTP(gmt, period);    // time zone in hours and refresh period in seconds
```

> Starting with version 2.1.0, a global facility is available`NTP`You don't have to create your own.

<a id="usage"></a>

## Use of use
```cpp
// set time zone in hours or minutes (globally for Stamp)
void setGMT(int16_t gmt);

// set the update period in seconds
void setPeriod(uint16_t prd);

// turn on the asynchronous mode (by omission. true)
void asyncMode(bool async);

// install the host (silent.pool.ntp.org)
void setHost(const String& host);

// host
void setHost(const IPAddress& host);

// establish the port (in omission 123)
void setPort(uint16_t port);

// get ping NTP server, MS
int16_t ping();

// Return true when you change your online status
bool statusChanged();

// Do not take into account ping compounds (silent. false)
void ignorePing(bool ignore);

// plug in
void attachRTC(VirtualRTC& rtc);

// switch off RTC
void detachRTC();

// plug in
void onError(ErrorCallback cb);

// last mistake
Error getError();

// last mistake
const __FlashStringHelper* readError();

// bug
bool hasError();

// return true if the tick is waiting for a server response in asynchronous mode
bool busy();

// True - has an Internet connection
bool online();

// launch
bool begin();

// Start with time zone indication in hours or minutes (globally for Stamp)
bool begin(int16_t gmt);

// turn off NTP
void end();

// synchronously update the time from the server. true at success
bool updateNow();

// ticker, call the loop. True returns every second if synchronized. Synchronized by timer
bool tick();
```

## Features
- GyverNTP works with WiFi UDP for esp8266/esp32, but any other UDP client can be used (see below).
- There's a global facility.`NTP`how`Serial`, `Wire`And so on.
- We need to call.`tick()`main-cycle`loop()`It synchronizes time from the server on its timer and ensures the operation of the second timer.
- If the main cycle of the program is heavily loaded, and the time must be obtained with maximum accuracy (several ms), then you can turn off the asynchronous mode.`asyncMode(false)`
- The library continues to count the time after the synchronization is gone. According to my tests, esp “goes away” by ~1.7 seconds per day, so the standard synchronization period is chosen 1 hour.
- Inheriting class`StampKeeper`, which provides timekeeping, second timer operation and convenient time conversion

### Time zone
The time zone is set for all Stamp/Datime operations in the program! Setting a time zone in an NTP object is equivalent to a call`setStampZone()`Setting up a **global* time zone for the Stamp library:

```cpp
void setup() {
    // plug in
    NTP.begin(3); // time-zone
}
```

### Minimum example
```cpp
#include <GyverNTP.h>

void setup() {
    // plug in
    NTP.begin(3); // time-zone
}

void loop() {
    NTP.tick();   // ticker
}
```

### Second timer
For the convenience of automating events by timer, a second timer is built into the library, it works in 0 milliseconds of every second. Under the condition of the timer, NTP is guaranteed to be synchronized and gives the correct time:

```cpp
void loop() {
  if (NTP.tick()) {
    // New second!
    Serial.println(NTP.toString());
  }

  // so
  // NTP.tick();
  // if (NTP.newSecond()) { }
}
```

You can also connect the handler for a second:
```cpp
void newSecond() {
  // code
}

void setup() {
  // plug in
  NTP.begin(3); // time-zone
  NTP.onSecond(newSecond);

  // so
  NTP.onSecond([](){
    // code
  });
}

void loop() {
  NTP.tick();
}
```

### Getting the time
GyverNTP inherits[StampConvert](https://github.com/GyverLibs/Stamp)That is, you can get time in many ways:

```cpp
// momentarily
if (NTP.tick()) {
  // lineage
  Serial.print(NTP.toString());  // NTP.timeToString(), NTP.dateToString()
  Serial.print(':');
  Serial.println(NTP.ms());  // + milliseconds of the current second. Inside, the tick is always 0.

  // deduction
  Datime dt = NTP;  // or Datime dt(NTP)
  dt.year;
  dt.second;
  dt.hour;
  dt.weekDay();
  dt.yearDay();
  // ... and other Datime methods and variables

  // Reading directly slower than output in Datime
  NTP.second();
  NTP.minute();
  NTP.year();
  // ... and other methods of StampConvert

  // comparison
  NTP == DaySeconds(12, 35, 0);            // Compared to DaySeconds (time is 12:35:00)
  NTP == 1738237474;                       // comparison
  NTP == Datime(2025, 1, 30, 14, 14, 30);  // comparison
}
```

### Synchronization mode
GyverNTP can be used **without ticker** - you need to manually call`updateNow`to synchronize time. In this case, the time will be counted simply from the moment of the last synchronization, the second handler will not work:

```cpp
void setup() {
  // plug in
  NTP.begin(3);     // time-zone
  NTP.updateNow();  // synchronize
}

void loop() {
  Serial.println(NTP.toString());
  delay(1000);
}
```

### Dissynchronization
If the synchronization period is very long or the system loses communication for a long time, the clock will be desynchronized and synchronized the next time you call the server. If it takes more than 1 second, the behavior will be as follows:
- If the internal clock “hurries” – the second timer will stop working until real time catches up with the internal clock.
- If the internal clock is “lagging behind”, the timer will be called each iteration of the loop with the addition of time until the internal time catches up with the real time.

This is done so that the synchronization does not lose seconds - the library will process every second and will not repeat, which is very important for automation algorithms.

### Online verification
NTP works on UDP - a very lightweight and "cheap" communication protocol, accessing the server takes almost no time. Thanks to this, NTP can be used to verify communication with the Internet – where a standard TCP client freezes for a few seconds, NTP will asynchronously report the loss of communication. Within GyverNTP, this can be used as follows:
```cpp
void setup() {
  // plug in
  NTP.begin(3);
  NTP.setPeriod(5); // synchronization every 5 seconds
}

void loop() {
  NTP.tick();

  // It will return true when you change status.
  if (NTP.statusChanged()) {
    Serial.println(NTP.online());
    // The online flag can be used to transfer to other libraries.
    // Like FastBot2.
    // bot.setOnline(NTP.online());
  }
}
```

### RTC connection
The library supports connecting an external RTC to synchronize time:
- If it is time to sync with NTP, but there is an error, the time will be synchronized with RTC.
- If it is successfully synchronized with NTP, then the current time will also be recorded in the RTC (no more than once).`GNTP_RTC_WRITE_PERIOD`Shut up. 1 hour

An RTC object must be an instance of a class.`VirtualRTC`from ready-made, for example`GyverDS3231Min`. You can write your own class to connect any real-time source.

<a id="example"></a>

## Example
### Full demo.
```cpp
#include <Arduino.h>
#include <GyverNTP.h>

void setup() {
    Serial.begin(115200);
    WiFi.begin("WIFI_SSID", "WIFI_PASS");
    while (WiFi.status() != WL_CONNECTED) delay(100);
    Serial.println("Connected");

    // error-handler
    NTP.onError([]() {
        Serial.println(NTP.readError());
        Serial.print("online: ");
        Serial.println(NTP.online());
    });

    // second handler (called from ticker)
    NTP.onSecond([]() {
        Serial.println("new second!");
    });

    // Synchronization processor (called from sync)
    // NTP.onSync([](uint32_t unix) {
    //     Serial.println("sync: ");
    //     Serial.print(unix);
    // });

    NTP.begin(3);                           // time-zone
    // NTP.setPeriod(30); // synchronization period in seconds
    // NTP.setHost ("ntp1.stratum2.ru") // install another host
    // NTP.setHost(IPAddress(1, 2, 3, 4)) // install another host
    // NTP.asyncMode(false) // turn off asynchronous mode
    // NTP.ignorePing(true) // Not counting ping to server
    // NTP.updateNow(); // Update right now
}

void loop() {
    // The ticker will return true every second in 0 ms of a second if the time is synchronized.
    if (NTP.tick()) {
        // lineage
        Serial.print(NTP.toString());  // NTP.timeToString(), NTP.dateToString()
        Serial.print(':');
        Serial.println(NTP.ms());  // + milliseconds of the current second. Inside, the tick is always 0.
    }

    if (NTP.newSecond()) {
        // You can catch another second here.
    }

    // Online status has changed
    if (NTP.statusChanged()) {
        Serial.print("STATUS: ");
        Serial.println(NTP.online());
    }
}
```

### RTC

```cpp
#include <Arduino.h>
#include <GyverNTP.h>

// GyverDS3231 supports GyverNTP
#include <GyverDS3231Min.h>
GyverDS3231Min rtc;

// You can write your class and use any other RTC.
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

    NTP.begin(3);  // time-zone

    // plug in
    // NTP.attachRTC(vrtc);
    NTP.attachRTC(rtc);
}

void loop() {
    if (NTP.tick()) {
        Serial.println(NTP.toString());
    }
}
```

<a id="versions"></a>

## Versions
- v1.0
- v1.1 - minor improvements and gmt in minutes
- v1.2 - Optimization, improved stability, added asynchronous mode
- v1.2.1 - The standard update period has been changed
- v1.3 - Accelerated synchronization when starting in asynchronous mode
- v1.3.1 - Included the WiFi library in a file
- v2.0 - Stamp dependency added, more features, online checkout for other libraries
- v2.1 - added global NTP object
- v2.2.0 - more stable operation, new features

<a id="install"></a>

## Installation
- The library can be found under the name **GyverNTP** and installed through the library manager in:
    - Arduino IDE
    - Arduino IDE v2
    - PlatformIO
- [Download the library](https://github.com/GyverLibs/GyverNTP/archive/refs/heads/main.zip).zip archive for manual installation:
    - Unpack and put in *C:\Program Files (x86)\Arduino\libraries* (Windows x64)
    - Unpack and put in *C:\Program Files\Arduino\libraries* (Windows x32)
    - Unpack and put in *Documents/Arduino/libraries/ *
    - (Arduino IDE) Automatic installation from .zip: *Sketch/Connect library/Add .ZIP library...* and specify downloaded archive
- Read more detailed instructions for installing libraries[here](https://alexgyver.ru/arduino-first/#%D0%A3%D1%81%D1%82%D0%B0%D0%BD%D0%BE%D0%B2%D0%BA%D0%B0_%D0%B1%D0%B8%D0%B1%D0%BB%D0%B8%D0%BE%D1%82%D0%B5%D0%BA)
### Update
- I recommend always updating the library: new versions fix errors and bugs, as well as optimize and add new features.
- Through the library manager IDE: find the library as when installing and click "Update"
- Manually: **Delete the folder with the old version** and then put the new one in its place. “Replacement” can not be done: sometimes new versions delete files that will remain when replaced and can lead to errors!

<a id="feedback"></a>

## Bugs and feedback
If you find bugs, create **Issue**, or better write to the mail immediately.[alex@alexgyver.ru](mailto:alex@alexgyver.ru)  
The library is open for revision and your **Pull Requests*!

When reporting bugs or incorrect work of the library, it is necessary to specify:
- Library version
- What is used by the IC
- SDK version (for ESP)
- Arduino IDE version
- Are embedded examples that use features and designs that cause bugs in your code working correctly?
- What code was downloaded, what work was expected from it and how it works in reality
- Ideally, attach the minimum code in which the bug is observed. Not a canvas of a thousand lines, but a minimum code.
