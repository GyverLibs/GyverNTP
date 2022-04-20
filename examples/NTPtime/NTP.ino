void NTPLoop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (timeStatus() == timeNotSet) {
      NTP();
    }
  }

  time_t t = now();
  if (hour(t) == 0 && minute(t) == 0 && second(t) >= 0) {
    NTP();
  }

  if (NTPUpdate) {
    NTPSet();
  }

  if (millis() - StartMlTime > 1000){ //щетчик милисекунд / збрасываеться для избижание проблем после переполнения millis()
    StartMlTime += 1000;
  }
}

void NTP() {
  if (!NTPUpdate) {
    NTPUpdate = true;

    byte packetBuffer[48];
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision

    IPAddress NTP_IP;

    if (WiFi.hostByName(NTPServerName, NTP_IP)) {

      Serial.println(F("sending NTP packet..."));

      NTP_UDP.beginPacket(NTP_IP, NTPport); //NTP requests are to port 123
      NTP_UDP.write(packetBuffer, 48);
      NTP_UDP.endPacket();

      UPDATETime = millis();
    } else NTPUpdate = false;
  }
}

void NTPSet() {
  byte packetBuffer[48];

  int size = NTP_UDP.parsePacket();
  if (size == 48 && NTP_UDP.remotePort() == NTPport) {
    Serial.println(F("NTP packet received"));

    // We've received a packet, read the data from it
    NTP_UDP.read(packetBuffer, 48); // read the packet into the buffer

    // this is NTP time (seconds since Jan 1 1900):

    //Serial.print("Seconds since Jan 1 1900 = ");
    //Serial.println((unsigned long)(packetBuffer[40] << 8 | packetBuffer[41]) << 16 | (packetBuffer[42] << 8 | packetBuffer[43]));

    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    //Serial.println((unsigned long)(packetBuffer[40] << 8 | packetBuffer[41]) << 16 | (packetBuffer[42] << 8 | packetBuffer[43])- 2208988800UL);

    uint32_t Time = (packetBuffer[40] << 8 | packetBuffer[41]) << 16 | (packetBuffer[42] << 8 | packetBuffer[43]);

    uint32_t TimeMl = ((packetBuffer[44] << 8 | packetBuffer[45]) << 16 | (packetBuffer[46] << 8 | packetBuffer[47])) / 4294967.296;

    uint32_t tDelay = ((millis() - UPDATETime) / 2) + TimeMl;

    uint16_t tAddS = tDelay / 1000;
    tAddS += 1;

    tDelay = 1000 - (tDelay % 1000);

    delay(tDelay);

    StartMlTime = millis();

    Time += TimeZone * SECS_PER_HOUR + tAddS - 2208988800UL;

    time_t LastTime = Time;
    Serial.print(F("Set time:"));
    Serial.print(hour(LastTime));
    Serial.print(':');
    Serial.print(minute(LastTime));
    Serial.print(':');
    Serial.print(second(LastTime));
    Serial.print('.');
    Serial.print(millis());
    Serial.print(" - ");

    Serial.print(day(LastTime));
    Serial.print('.');
    Serial.print(month(LastTime));
    Serial.print('.');
    Serial.print(year(LastTime));
    Serial.print(' ');
    Serial.println(dayStr(weekday(LastTime)));

    NTPUpdate = false;

    setTime(Time);
  }
  if (NTPUpdate && (millis() - UPDATETime >= 64000)) { //рядок 31 (Polling Interval) немного з сапасом
    NTPUpdate = false;
  }
}
