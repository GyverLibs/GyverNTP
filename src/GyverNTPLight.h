#pragma once

#include <StampKeeper.h>

#include "requestNTP.h"

#define NTPL_SYNC_PRD (60ul * 60 * 1000)  // период синхронизации
#define NTPL_SYNC_COLD_PRD (5ul * 1000)   // период холодной синхронизации
#define NTPL_TIMEOUT 2000                 // таймаут ожидания ответа сервера

class GyverNTPLight : public StampKeeper {
   public:
    GyverNTPLight(int16_t gmt = 0) {
        setStampZone(gmt);
    }

    // установить часовой пояс в часах или минутах (глобально для Stamp)
    void setGMT(int16_t gmt) {
        setStampZone(gmt);
    }

    // установить хост (умолч. "pool.ntp.org")
    void setHost(const char* host) {
        _host = host;
    }

    // установить порт (умолч. 123)
    void setPort(uint16_t port) {
        _port = port;
    }

    // тикер, вызывать в loop. Вернёт true на новой секунде
    bool tick() {
        if (millis() - _tmr >= (StampKeeper::synced() ? NTPL_SYNC_PRD : NTPL_SYNC_COLD_PRD)) {
            _tmr = millis();
            uint32_t unix = requestNTP(_host, _port, NTPL_TIMEOUT);
            if (unix) StampKeeper::sync(unix);
        }
        return StampKeeper::tick();
    }

   private:
    uint32_t _tmr = 0;
    const char* _host = "pool.ntp.org";
    uint16_t _port = 123;
};
