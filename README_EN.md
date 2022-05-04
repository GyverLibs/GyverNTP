This is an automatic translation, may be incorrect in some places. See sources and examples!

# GyverNTP
Library for getting exact time from NTP server for esp8266/esp32
- Powered by WiFiUdp.h standard library
- Accounting for server response time and connection delay
- Get time accurate to a few milliseconds
- Getting UNIX time, as well as milliseconds, seconds, minutes, hours, day, month, year and day of the week
- Synchronization by timer
- Error processing
- Asynchronous mode

### Compatibility
esp8266, esp32

## Content
- [Install](#install)
- [Initialization](#init)
- [Usage](#usage)
- [Example](#example)
- [Versions](#versions)
- [Bugs and feedback](#feedback)

<a id="install"></a>
## Installation
- The library can be found under the name **GyverNTP** and installed through the library manager in:
    - Arduino IDE
    - Arduino IDE v2
    - PlatformIO
- [Download library](https://github.com/GyverLibs/GyverNTP/archive/refs/heads/main.zip) .zip archive for manual installation:
    - Unzip and put in *C:\Program Files (x86)\Arduino\libraries* (Windows x64)
    - Unzip and put in *C:\Program Files\Arduino\libraries* (Windows x32)
    - Unpack and put in *Documents/Arduino/libraries/*
    - (Arduino IDE) automatic installation from .zip: *Sketch/Include library/Add .ZIP library…* and specify the downloaded archive
- Read more detailed instructions for installing libraries [here] (https://alexgyver.ru/arduino-first/#%D0%A3%D1%81%D1%82%D0%B0Cranberries D0%B5%D0%BA)

<a id="init"></a>
## Initialization
```cpp
GyverNTP ntp; // default parameters (gmt 0, period 3600 seconds (1 hour))
GyverNTP(gmt); // time zone in hours (for example, Moscow 3)
GyverNTP(gmt, period); // time zone in hours and update period in seconds
```

<a id="usage"></a>
## Usage
```cpp
boolbegin(); // run
void end(); // stop

void setGMTminute(int16_t gmt); // set timezone in minutes
void setGMT(int8_t gmt); // set timezone in hours
void setPeriod(uint16_tprd); // set update period in seconds
void setHost(char*host); // set host (default "pool.ntp.org")
void asyncMode(bool f); // asynchronous mode (enabled by default, true)
void ignorePing(bool f); // ignore connection ping (default false)

uint8_t tick(); // ticker, updates the time according to its timer. Returns true if an update was attempted
uint8_t updateNow(); // manually request and update the time from the server. Return status (see below)
uint32_t unix(); // unix time

uint16_tms(); // milliseconds of the current second
uint8_t second(); // get seconds
uint8_t minute(); // get minutes
uint8_t hour(); // get clock
uint8_tday(); // get the day of the month
uint8_t month(); // get the month
uint16_tyear(); // get the year
uint8_tdayWeek(); // get the day of the week

StringtimeString(); // get the time string in HH:MM:SS format
String dateString(); // get date string in DD.MM.YYYY format

bool synced(); // get current time status, true - synchronized
bool busy(); // will return true if tick is waiting for server response in asyncon mode
int16_t ping(); // get server ping
uint8_t status(); // get system status

// 0 - everything is ok
// 1 - UDP is not running
// 2 - WiFi not connected
// 3 - server connection error
// 4 - packet sending error
// 5 - server response timeout
// 6 - incorrect server response received
```

### Peculiarities
- You need to call `tick()` in the main loop of the program `loop()`, it synchronizes time according to its timer
- If the main loop of the program is heavily loaded, and the time needs to be obtained with maximum accuracy (several ms), then you can turn off the asynchronous mode `asyncMode(false)`
- The library continues to count the time even after the loss of synchronization
    - According to my tests, esp "leaves" by ~ 1.7 seconds per day (without synchronization). Therefore, the standard synchronization period is 1 hour.

<a id="example"></a>
## Example
```cpp
// example prints the time every second
// and also blinks the LED twice per second
// you can flash on several boards - they will flash synchronously

#include <ESP8266WiFi.h> // esp8266
//#include <WiFi.h> // esp32

#include <GyverNTP.h>
GyverNTP ntp(3);

// list of servers if "pool.ntp.org" is down
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
  //ntp.asyncMode(false); // disable asynchronous mode
  //ntp.ignorePing(true); // ignore ping to the server
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
```

<a id="versions"></a>
## Versions
- v1.0
- v1.1 - minor improvements and gmt in minutes
- v1.2 - optimization, improved stability, addeden asynchronous mode
- v1.2.1 - changed the standard update period

<a id="feedback"></a>
## Bugs and feedback
When you find bugs, create an **Issue**, or better, immediately write to the mail [alex@alexgyver.ru](mailto:alex@alexgyver.ru)
The library is open for revision and your **Pull Request**'s!