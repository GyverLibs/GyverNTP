#pragma once

#define GNTP_OFF_PERIOD 3000              // период опроса при оффлайн статусе, мс
#define GNTP_NTP_TIMEOUT 2500             // таймаут ожидания ответа от NTP сервера, мс
#define GNTP_LOCAL_PORT 1234              // локальный udp порт
#define GNTP_DEFAULT_HOST "pool.ntp.org"  // хост по умолчанию
#define GNTP_NTP_PORT 123                 // ntp порт

#include <StampTicker.h>
#include <Udp.h>

class GyverNTPClient : public StampTicker {
#ifdef __AVR__
    typedef void (*StatusHandler)();
    typedef void (*SyncHandler)();
#else
    typedef std::function<void()> StatusHandler;
    typedef std::function<void()> SyncHandler;
#endif

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
    GyverNTPClient(UDP& udp, int16_t gmt = 0, uint16_t prd = 3600) : udp(udp) {
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

    // запустить и установить часовой пояс в часах или минутах
    bool begin(int16_t gmt) {
        setGMT(gmt);
        return begin();
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

    // вернёт true при изменении статуса
    bool statusChanged() {
        return _changed;
    }

    // ============== ТИКЕР ===============
    // тикер, обновляет время по своему таймеру. Вернёт true на каждой секунде, если синхронизирован
    bool tick() {
        if (_changed) _changed = 0;

        if (_UDP_ok) {
            if (WiFi.status() != WL_CONNECTED) {
                if (_stat != Status::NoWiFi) {
                    _error(Status::NoWiFi);
                }
            }

            if (WiFi.status() == WL_CONNECTED) {
                if (_async) {
                    if (!_busy) {
                        if (_timeToUpdate()) {
                            _busy = _sendPacket();
                            _change();
                        }
                    } else {
                        if (millis() - _rtt > GNTP_NTP_TIMEOUT) {
                            _error(Status::ResponseTimeout);
                            _busy = false;
                        }
                        if (udp.parsePacket() == 48) {
                            _readPacket();
                            _busy = false;
                            _change();
                        }
                    }
                } else {
                    if (_timeToUpdate()) {
                        updateNow();
                        _change();
                    }
                }
            }
        }

        return StampTicker::tick();
    }

    // вручную запросить и обновить время с сервера. true при успехе
    bool updateNow() {
        if (!_UDP_ok || WiFi.status() != WL_CONNECTED) return _error(Status::NoWiFi);
        if (!_sendPacket()) return 0;

        while (udp.parsePacket() != 48) {
            if (millis() - _rtt > GNTP_NTP_TIMEOUT) return _error(Status::ResponseTimeout);
            delay(0);
        }
        return _readPacket();
    }

    // подключить обработчик смены статуса
    void attachStatus(StatusHandler cb) {
        _stat_cb = cb;
    }

    // отключить обработчик смены статуса
    void detachStatus() {
        _stat_cb = nullptr;
    }

    // подключить обработчик первой успешной синхронизации
    void attachSync(SyncHandler cb) {
        _sync_cb = cb;
    }

    // отключить обработчик первой успешной синхронизации
    void detachSync() {
        _sync_cb = nullptr;
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
    UDP& udp;
    String _host = GNTP_DEFAULT_HOST;
    uint32_t _tmr = 0;
    uint32_t _prd = 0;
    uint32_t _rtt = 0;
    StatusHandler _stat_cb = nullptr;
    SyncHandler _sync_cb = nullptr;

    int16_t _ping = 0;
    Status _stat = Status::NoUDP;
    bool _UDP_ok = false;
    bool _online = false;
    bool _busy = false;
    bool _usePing = true;
    bool _async = true;
    bool _changed = false;
    bool _first = true;

    using StampTicker::update;

    bool _sendPacket() {
        uint8_t buf[48] = {0b11100011};  // LI 0x3, v4, client (https://ru.wikipedia.org/wiki/NTP)
        if (!udp.beginPacket(_host.c_str(), GNTP_NTP_PORT)) return _error(Status::ConnectionError);
        if (udp.write(buf, 48) != 48 || !udp.endPacket()) return _error(Status::RequestError);
        _rtt = millis();
        return _stat = Status::OK, 1;
    }

    bool _readPacket() {
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
        if (_first) {
            _first = false;
            if (_sync_cb) _sync_cb();
        }
        return _stat = Status::OK, 1;
    }

    inline uint32_t _merge(uint8_t* buf) {
        return (buf[0] << 8) | buf[1];
    }

    bool _error(Status err) {
        _stat = err;
        _online = false;
        _change();
        return 0;
    }

    bool _timeToUpdate() {
        if (!_tmr || millis() - _tmr >= (_online ? _prd : GNTP_OFF_PERIOD)) {
            _tmr = millis();
            return 1;
        }
        return 0;
    }

    void _change() {
        if (_stat_cb) _stat_cb();
        _changed = true;
    }
};