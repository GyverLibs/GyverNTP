#pragma once
#include <StampKeeper.h>
#include <Udp.h>
#include <VirtualRTC.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#else
#error "No implementation for given platform"
#endif

#define GNTP_MIN_PERIOD (5ul * 1000)              // минимальный период синхронизации
#define GNTP_DNS_PERIOD (10ul * 60 * 1000)        // период обновления DNS
#define GNTP_OFFLINE_DNS_PERIOD (15 * 1000ul)     // период опроса при первом оффлайн статусе, мс
#define GNTP_OFFLINE_PERIOD (4 * 1000ul)          // период опроса при оффлайн статусе, мс
#define GNTP_RTC_WRITE_PERIOD (60ul * 60 * 1000)  // период записи в RTC, мс
#define GNTP_NTP_TIMEOUT (2500ul)                 // таймаут ожидания ответа от NTP сервера, мс
#define GNTP_LOCAL_PORT (1234)                    // локальный udp порт
#define GNTP_NTP_PORT 123                         // ntp порт по умолчанию
#define GNTP_NTP_HOST "pool.ntp.org"              // ntp хост по умолчанию

class GyverNTPClient : public StampKeeper {
    typedef std::function<void()> ErrorCallback;

    enum class State : uint8_t {
        Disabled,
        Idle,
        WaitPacket,
    };

   public:
    enum class Error : uint8_t {
        None,
        WiFi,
        Timeout,
        Host,
        Begin,
        BeginPacket,
        SendPacket,
        ParsePacket,
    };

    // конструктор (часовой пояс в часах или минутах, период обновления в секундах)
    GyverNTPClient(UDP& udp, int16_t gmt = 0, uint16_t prd = 3600) : udp(udp) {
        setGMT(gmt);
        setPeriod(prd);
    }

    // установить часовой пояс в часах или минутах (глобально для Stamp)
    void setGMT(int16_t gmt) {
        setStampZone(gmt);
    }

    // установить период обновления в секундах
    void setPeriod(uint16_t prd) {
        _prd = prd * 1000ul;
        if (_prd < GNTP_MIN_PERIOD) _prd = GNTP_MIN_PERIOD;
    }

    // включить асинхронный режим (по умолч. true)
    void asyncMode(bool async) {
        _async = async;
    }

    // установить хост (умолч. "pool.ntp.org")
    void setHost(const String& host) {
        _host = host;
        _host_ip = INADDR_NONE;
    }

    // установить хост IP
    void setHost(const IPAddress& host) {
        _host = "";
        _host_ip = host;
    }

    // установить порт (умолч. 123)
    void setPort(uint16_t port) {
        _port = port;
    }

    // получить пинг NTP сервера, мс
    int16_t ping() {
        return _ping;
    }

    // вернёт true при изменении статуса online
    bool statusChanged() {
        return _changed;
    }

    // не учитывать пинг соединения (умолч. false)
    void ignorePing(bool ignore) {
        _usePing = ignore;
    }

    // подключить RTC
    void attachRTC(VirtualRTC& rtc) {
        _rtc = &rtc;
    }

    // отключить RTC
    void detachRTC() {
        _rtc = nullptr;
    }

    // подключить обработчик ошибки
    void onError(ErrorCallback cb) {
        _state_cb = cb;
    }

    // отключить обработчик ошибки
    void detachError() {
        _state_cb = nullptr;
    }

    // получить последнюю ошибку
    Error getError() {
        return _error;
    }

    // получить последнюю ошибку
    const __FlashStringHelper* readError() {
        switch (_error) {
            case Error::None: return F("None");
            case Error::WiFi: return F("WiFi");
            case Error::Timeout: return F("Timeout");
            case Error::Host: return F("Host");
            case Error::Begin: return F("Begin");
            case Error::BeginPacket: return F("BeginPacket");
            case Error::SendPacket: return F("SendPacket");
            case Error::ParsePacket: return F("ParsePacket");
        }
        return F("");
    }

    // есть ошибка
    bool hasError() {
        return _error != Error::None;
    }

    // вернёт true, если tick ожидает ответа сервера в асинхронном режиме
    bool busy() {
        return _state == State::WaitPacket;
    }

    // true - есть соединение с Интернет
    bool online() {
        return _online;
    }

    // запустить
    bool begin() {
        if (_state == State::Disabled) {
            if (!udp.begin(GNTP_LOCAL_PORT)) return _setError(Error::Begin);

            _state = State::Idle;
            _error = Error::None;
            _ping = 0;
            _changed = false;
            _online = false;
            _first = true;
            // _rtc_synced = false;
            _sync_tmr.reset();
            _dns_tmr.reset();
            _rtc_r_tmr.reset();
            _rtc_w_tmr.reset();
            if (_host.length()) _host_ip = INADDR_NONE;
            if (_state_cb) _state_cb();
            StampKeeper::reset();
            return true;
        }
        return false;
    }

    // запустить с указанием часового пояса в часах или минутах (глобально для Stamp)
    bool begin(int16_t gmt) {
        setGMT(gmt);
        return begin();
    }

    // выключить NTP
    void end() {
        if (_state != State::Disabled) {
            _setError(Error::None);
            _state = State::Disabled;
            udp.stop();
        }
    }

    // синхронно обновить время с сервера. true при успехе
    bool updateNow() {
        if (_state == State::Disabled) return false;

        if (!_sendPacket()) return false;

        while (!_available()) {
            if (millis() - _rtt >= GNTP_NTP_TIMEOUT) return _setError(Error::Timeout);
            delay(1);
        }

        return _readPacket();
    }

    // тикер, вызывать в loop. Вернёт true каждую секунду, если синхронизирован. Синхронизируется по таймеру
    bool tick() {
        if (_changed) _changed = false;
        if (_state != State::Disabled) {
            if (_first && _connected()) {
                _first = false;
                _sync_tmr.reset();
            }
            if (_async) {
                switch (_state) {
                    case State::Idle:
                        if (_timeToSync()) _sendPacket();
                        break;

                    case State::WaitPacket:
                        if (_connected()) {
                            if (_available()) _readPacket();
                            else if (millis() - _rtt > GNTP_NTP_TIMEOUT) _setError(Error::Timeout);
                        } else {
                            _setError(Error::WiFi);
                        }
                        break;

                    default: break;
                }
            } else {
                if (_timeToSync()) updateNow();
            }
        }
        return StampKeeper::tick();
    }

   private:
    class Timer {
       public:
        bool elapsed(uint32_t prd) {
            if (!_tmr || millis() - _tmr >= prd) {
                restart();
                return true;
            }
            return false;
        }
        void restart() {
            _tmr = millis();
            if (!_tmr) --_tmr;
        }
        void reset() {
            _tmr = 0;
        }

       private:
        uint32_t _tmr = 0;
    };

    UDP& udp;
    VirtualRTC* _rtc = nullptr;
    uint32_t _prd = 0;
    uint32_t _rtt = 0;
    Timer _sync_tmr;
    Timer _dns_tmr;
    Timer _rtc_r_tmr;
    Timer _rtc_w_tmr;
    ErrorCallback _state_cb = nullptr;
    String _host = GNTP_NTP_HOST;
    IPAddress _host_ip;
    uint16_t _port = GNTP_NTP_PORT;
    int16_t _ping = 0;
    State _state = State::Disabled;
    Error _error = Error::None;
    bool _usePing = true;
    bool _online = false;
    bool _async = true;
    bool _changed = false;
    bool _first = true;
    // bool _rtc_synced = false;

    bool _connected() {
        return WiFi.status() == WL_CONNECTED;
    }

    bool _timeToSync() {
        return (_sync_tmr.elapsed(_host_ip ? (_online ? _prd : GNTP_OFFLINE_PERIOD) : GNTP_OFFLINE_DNS_PERIOD));
    }

    bool _getHost() {
        if (_host.length()) {
            if (!_host_ip || _dns_tmr.elapsed(GNTP_DNS_PERIOD)) {
                _dns_tmr.restart();
                IPAddress host;
                if (WiFi.hostByName(_host.c_str(), host)) _host_ip = host;
            }
        }
        return _host_ip;
    }

    bool _sendPacket() {
        uint8_t buf[48] = {0b11100011};
        if (!_connected()) return _setError(Error::WiFi);
        if (!_getHost()) return _setError(Error::Host);
        if (!udp.beginPacket(_host_ip, _port)) return _setError(Error::BeginPacket);
        if (!(udp.write(buf, 48) == 48 && udp.endPacket())) return _setError(Error::SendPacket);

        _state = State::WaitPacket;
        _rtt = millis();
        return true;
    }

    bool _available() {
        return (udp.parsePacket() == 48 && udp.remotePort() == _port);
    }

    bool _readPacket() {
        _rtt = millis() - _rtt;  // время между запросом и ответом
        uint8_t buf[48];
        if (!(udp.read(buf, 48) == 48 && buf[40])) return _setError(Error::ParsePacket);

        uint16_t a_ms = (_merge(buf + 44) * 1000) >> 16;      // мс ответа сервера
        if (_usePing) {                                       // расчёт пинга
            uint16_t r_ms = (_merge(buf + 36) * 1000) >> 16;  // мс запроса клиента
            int16_t err = a_ms - r_ms;                        // время обработки сервером
            if (err < 0) err += 1000;                         // переход через секунду
            _rtt = (_rtt - err) / 2;                          // текущий пинг
            _ping = _ping ? (_ping + _rtt) / 2 : _rtt;        // бегущее среднее по двум
            a_ms += _ping;
        }
        uint32_t unix = ((_merge(buf + 40) << 16) | _merge(buf + 42)) - 2208988800ul;  // 1900 to unix

        // пропускать тики если подключен и не синхронизирован rtc
        StampKeeper::sync(unix, a_ms /*, (_rtc && !_rtc_synced)*/);

        if (!_online) _changed = true;
        _online = true;

        _state = State::Idle;
        _error = Error::None;
        if (_state_cb) _state_cb();
        if (_rtc && _rtc_w_tmr.elapsed(GNTP_RTC_WRITE_PERIOD)) {
            _rtc->setUnix(unix);
            // _rtc_synced = true;
        }
        return true;
    }

    inline uint32_t _merge(uint8_t* buf) {
        return (buf[0] << 8) | buf[1];
    }

    bool _setError(Error error) {
        if (_online && error == Error::Timeout) _dns_tmr.reset();

        if (_online) _changed = true;
        _online = false;

        if (_rtc && _rtc_r_tmr.elapsed(_prd)) {
            // пропускать тики если не синхронизирован rtc
            StampKeeper::sync(_rtc->getUnix() /*, 0, !_rtc_synced*/);
        }
        _error = error;
        _state = State::Idle;
        if (_state_cb) _state_cb();
        return false;
    }
};