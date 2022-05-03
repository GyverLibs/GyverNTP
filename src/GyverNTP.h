/*
    Библиотека для получения точного времени с NTP сервера для esp8266/esp32
    GitHub: https://github.com/GyverLibs/GyverNTP
    - Работает на стандартной библиотеке WiFiUdp.h
    - Учёт времени ответа сервера и задержки соединения
    - Получение времени с точностью до нескольких миллисекунд
    - Получение UNIX-времени, а также миллисекунд, секунд, минут, часов, дня, месяца, года и дня недели
    - Синхронизация по таймеру
    - Обработка ошибок
    - Асинхронный режим
    
    AlexGyver, alex@alexgyver.ru
    https://alexgyver.ru/
    MIT License
    
    v1.0
    v1.1 - мелкие улучшения и gmt в минутах
    v1.2 - оптимизация, улучшена стабильность, добавлен асинхронный режим
    v1.2.1 - изменён стандартный период обновления
*/

#ifndef _GyverNTP_h
#define _GyverNTP_h

#define GN_NTP_TIMEOUT 3000
#define GN_LOCAL_PORT 1234
#define GN_DEFAULT_HOST "pool.ntp.org"
#define GN_NTP_PORT 123

#include <WiFiUdp.h>

class GyverNTP {
public:
    // конструктор
    GyverNTP(int8_t gmt = 0, uint16_t prd = 3600) {
        setGMT(gmt);
        setPeriod(prd);
    }
    
    // установить часовой пояс в часах
    void setGMT(int16_t gmt) {
        _gmt = gmt * 3600L;
    }
    
    // установить часовой пояс в минутах
    void setGMTminute(int16_t gmt) {
        _gmt = gmt * 60L;
    }
    
    // установить период обновления в секундах
    void setPeriod(uint16_t prd) {
        _prd = prd * 1000ul;
    }
    
    // установить хост (по умолч. pool.ntp.org)
    void setHost(const char* host) {
        _host = host;
    }
    
    // запустить
    bool begin() {
        _tmr = millis();
        _stat = !udp.begin(GN_LOCAL_PORT);     // stat 0 - OK
        return !_stat;
    }
    
    // остановить
    void end() {
        udp.stop();
        _stat = 1;
    }
    
    // асинхронный режим (по умолч. включен, true)
    void asyncMode(bool f) {
        _async = f;
    }
    
    // ============== ТИКЕР ===============
    // тикер, обновляет время по своему таймеру
    bool tick() {
        if (_async) {
            if (!requested) {
                if (_stat != 1 && (millis() - _tmr >= _prd || !_tmr)) {
                    _tmr = millis();            // сброс таймера
                    checkLeap();                // смещаем время
                    sendPacket();               // запрос NTP
                    if (!_stat) requested = 1;  // успешно
                    return 1;
                }
            } else {
                if (millis() - rtt > GN_NTP_TIMEOUT) {
                    _stat = 5;
                    requested = 0;
                    return 1;
                }
                if (udp.parsePacket() == 48) {
                    readPacket();
                    requested = 0;
                    return 1;
                }
            }
        } else {
            if (_stat != 1 && (millis() - _tmr >= _prd || !_tmr)) {
                _tmr = millis();
                updateNow();
                return 1;
            }
        }
        return 0;
    }
    
    // вручную запросить и обновить время с сервера. Вернёт статус (см. ниже)
    uint8_t updateNow() {
        if (_stat == 1) return 1;       // клиент не запущен
        checkLeap();                    // смещаем время
        sendPacket();                   // запрос
        if (_stat) return _stat;        // возврат ошибки (> 0)
        while (udp.parsePacket() != 48) {
            if (millis() - rtt > GN_NTP_TIMEOUT) return 5;
            yield();
        }
        return readPacket();
    }
    
    // миллисекунд с последнего обновления (с 0 секунд unix)
    uint32_t msFromUpdate() {
        return millis() - _last_upd;
    }
    
    // unix время
    uint32_t unix() {
        return _unix + msFromUpdate() / 1000ul;
    }

    // миллисекунды текущей секунды
    uint16_t ms() {
        return msFromUpdate() % 1000;
    }
    
    // получить секунды
    uint8_t second() {
        return (unix() + _gmt) % 60ul;
    }
    
    // получить минуты
    uint8_t minute() {
        return ((unix() + _gmt) % 3600ul) / 60ul;
    }
    
    // получить часы
    uint8_t hour() {
        return ((unix() + _gmt) % 86400ul) / 3600ul;
    }
    
    // получить день месяца
    uint8_t day() {
        updateTime();
        return _day;
    }
    
    // получить месяц
    uint8_t month() {
        updateTime();
        return _month;
    }
    
    // получить год
    uint16_t year() {
        updateTime();
        return _year;
    }
    
    // получить день недели
    uint8_t dayWeek() {
        updateTime();
        return _dayw;
    }
    
    // получить строку времени формата ЧЧ:ММ:СС
    String timeString() {
        String str;
        if (!_sync) return str = F("Not sync");
        str.reserve(8);
        if (hour() < 10) str += '0';
        str += hour();
        str += ':';
        if (minute() < 10) str += '0';
        str += minute();
        str += ':';
        if (second() < 10) str += '0';
        str += second();
        return str;
    }
    
    // получить строку даты формата ДД.ММ.ГГГГ
    String dateString() {
        String str;
        if (!_sync) return str = F("Not sync");
        str.reserve(10);
        if (day() < 10) str += '0';
        str += day();
        str += '.';
        if (month() < 10) str += '0';
        str += month();
        str += '.';
        str += year();    
        return str;
    }
    
    // получить пинг сервера
    int16_t ping() {
        return _ping;
    }
    
    // не учитывать пинг соединения
    void ignorePing(bool f) {
        _ignorePing = f;
    }
    
    // вернёт true, если tick ожидает ответа сервера в асинхронном режиме
    bool busy() {
        return requested;
    }
    
    // получить статус текущего времени, true - синхронизировано
    bool synced() {
        return _sync;
    }
    
    // получить статус системы
    /*
        0 - всё ок
        1 - не запущен UDP
        2 - не подключен WiFi
        3 - ошибка подключения к серверу
        4 - ошибка отправки пакета
        5 - таймаут ответа сервера
        6 - получен некорректный ответ сервера
    */
    uint8_t status() {
        return _stat;
    }
    
    uint8_t requestTime() {
        return updateNow();
    }
    
private:
    // пересчёт unix во время и дату и буферизация
    void updateTime() {
        if (millis() - _prev_calc < 300) return;
        _prev_calc = millis();
        // http://howardhinnant.github.io/date_algorithms.html#civil_from_days
        uint32_t u = (unix() + _gmt) / 86400ul;
        _dayw = (u + 4) % 7;
        if (!_dayw) _dayw = 7;
        u += 719468;
        uint8_t era = u / 146097ul;
        uint16_t doe = u - era * 146097ul;
        uint16_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
        _year = yoe + era * 400;
        uint16_t doy = doe - (yoe * 365 + yoe / 4 - yoe / 100);
        uint16_t mp = (doy * 5 + 2) / 153;
        _day = doy - (mp * 153 + 2) / 5 + 1;
        _month = mp + (mp < 10 ? 3 : -9);
        _year += (_month <= 2);
    }
    
    uint8_t sendPacket() {
        if (WiFi.status() != WL_CONNECTED) return _stat = 2;
        uint8_t buf[48];
        memset(buf, 0, 48);
        // https://ru.wikipedia.org/wiki/NTP
        buf[0] = 0b11100011;                    // LI 0x3, v4, client
        if (!udp.beginPacket(_host, GN_NTP_PORT)) return _stat = 3;
        udp.write(buf, 48);
        if (!udp.endPacket()) return _stat = 4;
        rtt = millis();
        return _stat = 0;
    }
    
    uint8_t readPacket() {
        if (udp.remotePort() != GN_NTP_PORT) return _stat = 6;     // не наш порт
        uint8_t buf[48];
        udp.read(buf, 48);                      // читаем
        if (buf[40] == 0) return _stat = 6;     // некорректное время
        rtt = millis() - rtt;                   // время между запросом и ответом
        _last_upd = millis();                   // запомнили время обновления
        if (!_sync) _sync = 1;                  // время синхронизировано
        uint16_t a_ms = ((buf[44] << 8) | buf[45]) * 1000L >> 16;    // мс ответа сервера
        if (!_ignorePing) {
            uint16_t r_ms = ((buf[36] << 8) | buf[37]) * 1000L >> 16;    // мс запроса клиента
            int16_t err = a_ms - r_ms;          // время обработки сервером
            if (err < 0) err += 1000;           // переход через секунду
            rtt = (rtt - err) / 2;              // текущий пинг
            if (_ping == 0) _ping = rtt;        // первая итерация
            else _ping = (_ping + rtt) / 2;     // бегущее среднее по двум
            _last_upd -= _ping;                 // учитываем пинг
        }
        _last_upd -= a_ms;                      // смещение для дальнейших расчётов
        _unix = (((uint32_t)buf[40] << 24) | ((uint32_t)buf[41] << 16) | ((uint32_t)buf[42] << 8) | buf[43]);    // 1900
        _unix -= 2208988800ul;                  // перевод в UNIX (1970)
        return _stat = 0;
    }
    
    // защита от переполнения разности через 50 суток
    void checkLeap() {
        uint32_t diff = millis() - _last_upd;
        if (_unix && diff > 86400000ul) {
            _unix += diff / 1000ul;
            _last_upd = millis() - diff % 1000ul;
        }
    }
    
    WiFiUDP udp;
    const char* _host = GN_DEFAULT_HOST;
    uint32_t _prev_calc = 0;
    uint32_t _last_upd = 0;
    uint32_t _tmr;
    uint32_t _prd;
    uint32_t _unix = 0;
    uint32_t rtt;
    
    int32_t _gmt = 0;
    int16_t _ping = 0;
    uint8_t _stat = 5;
    bool _sync = 0;
    bool requested = 0;
    bool _ignorePing = 0;
    bool _async = 1;

    uint8_t _day, _month, _dayw;
    int16_t _year;
};
#endif