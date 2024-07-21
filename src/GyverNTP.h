#pragma once

#define GNTP_OFF_PERIOD 3000              // период опроса при оффлайн статусе, мс
#define GNTP_NTP_TIMEOUT 2500             // таймаут ожидания ответа от NTP сервера, мс
#define GNTP_LOCAL_PORT 1234              // локальный udp порт
#define GNTP_DEFAULT_HOST "pool.ntp.org"  // хост по умолчанию
#define GNTP_NTP_PORT 123                 // ntp порт

#include <StampTicker.h>
#include <WiFiUdp.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#else
#error "No implementation for given platform"
#endif

class GyverNTP : public StampTicker {
   public:
    enum Status {
        OK,               // всё ок
        NoUDP,            // не запущен UDP
        NoWiFi,           // не подключен WiFi
        ConnectionError,  // ошибка подключения к серверу
        RequestError,     // ошибка отправки пакета
        ResponseTimeout,  // таймаут ответа сервера
        ResponseError,    // получен некорректный ответ сервера
    };

    // конструктор (часовой пояс в часах или минутах, период обновления в секундах)
    GyverNTP(int16_t gmt = 0, uint16_t prd = 3600) {
        setGMT(gmt);
        setPeriod(prd);
    }

    // установить часовой пояс в часах или минутах
    void setGMT(int16_t gmt) {
        setStampZone(gmt);
    }

    // установить период обновления в секундах
    void setPeriod(uint16_t prd) {
        if (prd < GNTP_NTP_TIMEOUT) prd = GNTP_NTP_TIMEOUT + 100;
        _prd = prd * 1000ul;
    }

    // установить хост (умолч. "pool.ntp.org")
    void setHost(const String& host) {
        _host = host;
    }

    // запустить
    bool begin() {
        _tmr = 0;
        _UDP_ok = udp.begin(GNTP_LOCAL_PORT);
        if (_UDP_ok) return _stat = Status::OK, 1;
        else return _error(Status::NoUDP);
    }

    // остановить
    void end() {
        udp.stop();
        _UDP_ok = false;
        _error(Status::NoUDP);
    }

    // включить асинхронный режим (по умолч. true)
    void asyncMode(bool async) {
        _async = async;
        _busy = 0;
    }

    // получить пинг сервера, мс
    int16_t ping() {
        return _ping;
    }

    // не учитывать пинг соединения (умолч. false)
    void ignorePing(bool ignore) {
        _usePing = ignore;
    }

    // вернёт true, если tick ожидает ответа сервера в асинхронном режиме
    bool busy() {
        return _busy;
    }

    // получить статус последнего действия
    Status status() {
        return _stat;
    }

    // true - не было ошибок связи и есть соединение с Интернет
    bool online() {
        return _online;
    }

    // ============== ТИКЕР ===============
    // тикер, обновляет время по своему таймеру. Вернёт true при смене статуса
    bool tick() {
        StampTicker::tick();

        if (!_UDP_ok || WiFi.status() != WL_CONNECTED) {
            if (_stat != Status::NoWiFi) {
                _error(Status::NoWiFi);
                return 1;
            }
            return 0;
        }

        if (_async) {
            if (!_busy) {
                if (_timeToUpdate()) {
                    _busy = sendPacket();
                    return 1;
                }
            } else {
                if (millis() - _rtt > GNTP_NTP_TIMEOUT) {
                    _error(Status::ResponseTimeout);
                    _busy = false;
                    return 1;
                }
                if (udp.parsePacket() == 48) {
                    readPacket();
                    _busy = false;
                    return 1;
                }
            }
        } else {
            if (_timeToUpdate()) {
                updateNow();
                return 1;
            }
        }
        return 0;
    }

    // вручную запросить и обновить время с сервера. true при успехе
    bool updateNow() {
        if (!_UDP_ok || WiFi.status() != WL_CONNECTED) return _error(Status::NoWiFi);
        if (!sendPacket()) return 0;

        while (udp.parsePacket() != 48) {
            if (millis() - _rtt > GNTP_NTP_TIMEOUT) return _error(Status::ResponseTimeout);
            delay(0);
        }
        return readPacket();
    }

    // ============ DEPRECATED ============

    // установить часовой пояс в минутах
    void setGMTminute(int16_t gmt) __attribute__((deprecated)) {
        setStampZone(gmt);
    }

    // получить строку времени формата ЧЧ:ММ:СС
    String timeString() __attribute__((deprecated)) {
        return timeToString();
    }

    // получить строку даты формата ДД.ММ.ГГГГ
    String dateString() __attribute__((deprecated)) {
        return dateToString();
    }

   private:
    using StampTicker::update;

    bool sendPacket() {
        uint8_t buf[48] = {0b11100011};  // LI 0x3, v4, client (https://ru.wikipedia.org/wiki/NTP)
        if (!udp.beginPacket(_host.c_str(), GNTP_NTP_PORT)) return _error(Status::ConnectionError);
        if (udp.write(buf, 48) != 48 || !udp.endPacket()) return _error(Status::RequestError);
        _rtt = millis();
        return _stat = Status::OK, 1;
    }

    bool readPacket() {
        if (udp.remotePort() != GNTP_NTP_PORT) return _error(Status::ResponseError);  // не наш порт
        uint8_t buf[48];
        if (udp.read(buf, 48) != 48 || !buf[40]) return _error(Status::ResponseError);  // некорректное время

        _rtt = millis() - _rtt;                               // время между запросом и ответом
        uint16_t a_ms = (_merge(buf + 44) * 1000) >> 16;      // мс ответа сервера
        if (_usePing) {                                       // расчёт пинга
            uint16_t r_ms = (_merge(buf + 36) * 1000) >> 16;  // мс запроса клиента
            int16_t err = a_ms - r_ms;                        // время обработки сервером
            if (err < 0) err += 1000;                         // переход через секунду
            _rtt = (_rtt - err) / 2;                          // текущий пинг
            if (_ping == 0) _ping = _rtt;                     // первая итерация
            else _ping = (_ping + _rtt) / 2;                  // бегущее среднее по двум
            a_ms += _ping;
        }
        uint32_t unix = ((_merge(buf + 40) << 16) | _merge(buf + 42)) - 2208988800ul;  // 1900 to unix
        update(unix, a_ms);

        _online = true;
        return _stat = Status::OK, 1;
    }

    inline uint32_t _merge(uint8_t* buf) {
        return (buf[0] << 8) | buf[1];
    }

    bool _error(Status err) {
        _stat = err;
        _online = false;
        return 0;
    }

    bool _timeToUpdate() {
        if (!_tmr || millis() - _tmr >= (_online ? _prd : GNTP_OFF_PERIOD)) {
            _tmr = millis();
            return 1;
        }
        return 0;
    }

    WiFiUDP udp;
    String _host = GNTP_DEFAULT_HOST;
    uint32_t _tmr = 0;
    uint32_t _prd = 0;
    uint32_t _rtt = 0;

    int16_t _ping = 0;
    Status _stat = Status::NoUDP;
    bool _UDP_ok = false;
    bool _online = false;
    bool _busy = false;
    bool _usePing = true;
    bool _async = true;
};