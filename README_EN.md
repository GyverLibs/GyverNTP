This is an automatic translation, may be incorrect in some places. See sources and examples!

# Gyverntp
Library for receiving the exact time with the NTP server for ESP8266/ESP32
- works at the standard library wifiudp.h
- taking into account the time of the server response and the delay of the connection
- Getting time up to several milliseconds
- receipt of unix time, as well as milliseconds, seconds, minutes, hours, day, month, year and day of the week
- Synchronization by timer
- Error processing
- Asynchronous regime

## compatibility
ESP8266, ESP32

## Content
- [installation] (# Install)
- [initialization] (#init)
- [use] (#usage)
- [Example] (# Example)
- [versions] (#varsions)
- [bugs and feedback] (#fedback)

<a id="install"> </a>
## Installation
- The library can be found by the name ** gyverntp ** and installed through the library manager in:
    - Arduino ide
    - Arduino ide v2
    - Platformio
- [download the library] (https://github.com/gyverlibs/gyverntp/archive/refs/heads/main.zip) .Zip archive for manual installation:
    - unpack and put in * C: \ Program Files (X86) \ Arduino \ Libraries * (Windows X64)
    - unpack and put in * C: \ Program Files \ Arduino \ Libraries * (Windows X32)
    - unpack and put in *documents/arduino/libraries/ *
    - (Arduino id) Automatic installation from. Zip: * sketch/connect the library/add .Zip library ... * and specify downloaded archive
- Read more detailed instructions for installing libraries [here] (https://alexgyver.ru/arduino-first/#%D0%A3%D1%81%D1%82%D0%B0%BD%D0%BE%BE%BE%BED0%B2%D0%BA%D0%B0_%D0%B1%D0%B8%D0%B1%D0%BB%D0%B8%D0%BE%D1%82%D0%B5%D0%BA)
### Update
- I recommend always updating the library: errors and bugs are corrected in the new versions, as well as optimization and new features are added
- through the IDE library manager: find the library how to install and click "update"
- Manually: ** remove the folder with the old version **, and then put a new one in its place.“Replacement” cannot be done: sometimes in new versions, files that remain when replacing are deleted and can lead to errors!


<a id="init"> </a>
## initialization
`` `CPP
Gyverntp NTP;// default parameters (GMT 0, period 3600 seconds (1 hour))
Gyverntp NTP (GMT);// hourly belt in watches (for example, Moscow 3)
Gyverntp NTP (GMT, Period);// Tax belt in the clock and the renewal period in seconds
`` `

<a id="usage"> </a>
## Usage
`` `CPP
Bool Begin ();// Launch
VOID end ();// Stop

VOID Setgmtminute (Int16_T GMT);// Install an hourly belt in minutes
VOID Setgmt (Int8_T GMT);// Install an hourly waist in the watch
VOID Setperiod (Uint16_T PRD);// set the update period in seconds
VOID Sethost (Char* Host);// Install the host (by default "" Pool.ntp.org ")
Void asyncmode (Bool F);// asynchronous regime (on the silence, turned on, true)
VOID ignoreping (bool f);// Do not take into account the ping of the connection (silence. FALSE)

uint8_t tick ();// Tiker, updates the time by its timer.Will return True if there is an attempt to update
uint8_t updatatenow ();// Manually request and update the time from the server.Will return the status (see below)
uint32_t unix ();// Unix Time

uint16_t ms ();// milliseconds of the current second
uint8_t second ();// Get second
uint8_t minute ();// Get minutes
uint8_t hor ();// Get a clock
uint8_t day ();// Get the day of the month
uint8_t month ();// Get a month
uint16_t year ();// Get a year
uint8_t Dayweek ();// Get the day of the week (Mon .. BC = 1 .. 7)

String Timestring ();// Get a line of time of the format of hhch: mm: ss
String Datestring ();// Get the line of the date of the format DD.MM.YYYY

Bool Synced ();// Get the status of the current time, True - synchronized
Bool Busy ();// will return True if Tick expects a server response in asynchronous mode
int16_t ping ();// Get server ping
uint8_t status ();// Get the status of the system

// 0 - everything is ok
// 1 - not launched UDP
// 2 - not connected to wifi
// 3 - error error to the server
// 4 - error sending a package
// 5 - server response timeout
// 6 - the incorrect server response was obtained
`` `

### Peculiarities
- We need to call `tick ()` in the main cycle of the `loop ()` program, it synchronizes the time according to its timer
- if the main cycle of the program is strongly loaded, and the time must be obtained with maximum accuracy (several MS), then you can turn off the asynchronous mode `asyncmode (false)` `
- The library continues to count time even after the loss of synchronization
    - according to my tests, ESP "goes" for ~ 1.7 seconds per day (without synchronization).Therefore, the standard period of synchronization is chosen 1 hour

<a id="EXAMPLE"> </a>
## Example
`` `CPP
// An example takes time every second
// And also twice a second flashes LED
// can be flashed on several boards - they will blink synchronously

#include <ESP8266WIFI.H> // ESP8266
//#include <wifi.h> // ESP32

#include <gyverntp.h>
Gyverntp NTP (3);

// List of servers if "Pool.ntp.org" does not work
//"ontp1.stratum2.ru "
//"ontp2.stratum2.ru "
//"ontp.msk-ig.ru "

VOID setup () {
  Pinmode (LED_BUILTIN, OUTPUT);
  Serial.Begin (115200);
  Wifi.begin ("wifi_ssid", "wifi_pass");
  While (wifi.status ()! = Wl_ConNECTED) DELAY (100);
  Serial.println ("connected");

  ntp.begin ();
  //NTP.ASYNCMODE(False);// turn off the asynchronous regime
  //ntp.ignoreping(true);// not to take into account the ping before the server
}

VOID loop () {
  ntp.tick ();
  
  if (ntp.ms () == 0) {
    DELAY (1);
    digitalWrite (LED_BUILTIN, 1);
  }
  if (ntp.ms () == 500) {
    DELAY (1);
    digitalWrite (LED_BUILTIN, 0);
    Serial.println (ntp.timestring ());
    Serial.println (ntp.datestestring ());
    Serial.println ();
  }
}
`` `

<a id="versions"> </a>
## versions
- V1.0
- V1.1 - Small improvements and GMT in minutes
- V1.2 - optimization, improved stability, added asynchronous regime
- v1.2.1 - The standard renewal period has been changed
- V1.3 - Synchronization is accelerated when launched in asynchronous mode
- v1.3.1 - struck the WiFi library to the file

<a id="feedback"> </a>
## bugs and feedback
Create ** Issue ** when you find the bugs, and better immediately write to the mail [alex@alexgyver.ru] (mailto: alex@alexgyver.ru)
The library is open for refinement and your ** pull Request ** 'ow!


When reporting about bugs or incorrect work of the library, it is necessary to indicate:
- The version of the library
- What is MK used
- SDK version (for ESP)
- version of Arduino ide
- whether the built -in examples work correctly, in which the functions and designs are used, leading to a bug in your code
- what code has been loaded, what work was expected from it and how it works in reality
- Ideally, attach the minimum code in which the bug is observed.Not a canvas of a thousand lines, but a minimum code