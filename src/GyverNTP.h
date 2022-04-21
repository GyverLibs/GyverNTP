/*
    Библиотека для получения точного времени с NTP сервера для esp8266/esp32
    GitHub: https://github.com/GyverLibs/GyverNTP
    - Работает на стандартной библиотеке WiFiUdp.h
    - Учёт времени ответа сервера и задержки соединения
    - Получение времени с точностью до нескольких миллисекунд
    - Получение UNIX-времени, а также миллисекунд, секунд, минут, часов, дня, месяца, года и дня недели
    - Синхронизация по таймеру
    - Обработка ошибок
    
    AlexGyver, alex@alexgyver.ru
    https://alexgyver.ru/
    MIT License
    
    v1.0
*/

#ifndef _GyverNTP_h
#define _GyverNTP_h

#define GN_NTP_TIMEOUT 64000
#define GN_LOCAL_PORT 1337
#define GN_DEFAULT_HOST "pool.ntp.org"
#define GN_NTP_PORT 123

#include <WiFiUdp.h>
static WiFiUDP _NTP_UDP;

class GyverNTP {
public:
    GyverNTP(int8_t gmt = 0, uint16_t prd = 360) {
        setGMT(gmt);
        setPeriod(prd);
    }
    
    // установить часовой пояс
    void setGMT(int8_t gmt) {
        _gmt = gmt;
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
    void begin() {
        _stat = !_NTP_UDP.begin(GN_LOCAL_PORT);     // stat 0 - OK
    }
    
    // остановить
    void end() {
        _NTP_UDP.stop();
        _stat = 1;
    }
    
    // тикер, обновляет время по своему таймеру
    uint8_t tick() {
        if (_stat != 1 && (millis() - _tmr >= _prd || !_tmr)) {
            _stat = requestTime();      // запрос NTP
        }
        return _stat;
    }
    
    // вручную запросить и обновить время с сервера
    uint8_t requestTime() {
		uint8_t buf[48];
		if (!_send_pack) {//проверяем не был ли отправлен запрос
			if (WiFi.status() != WL_CONNECTED) return 2;
			memset(buf, 0, 48);
			// https://ru.wikipedia.org/wiki/NTP
			buf[0] = 0b11100011;                    // LI 0x3, v4, client
			buf[2] = 6; 							//таймаут 2^6 = 64 секунд
			buf[3] = 0xEC; 							//точность -20 "Например, значению −18 будет соответствовать точность около 1 мкс." - Википедия
			if (!_NTP_UDP.beginPacket(_host, GN_NTP_PORT)) return 3;
			_NTP_UDP.write(buf, 48);
			if (!_NTP_UDP.endPacket()) return 4;
			rtt = millis();
			_send_pack = true;
		} 
		if (_send_pack) {//проверяем был ли отправлен запрос
			if (!_NTP_UDP.parsePacket()) {
				if (millis() - rtt > GN_NTP_TIMEOUT) {
					_send_pack = false;
					return 5;
				}
			} else {
				_ping = (millis() - rtt) /2;            // текущий пинг
				_NTP_UDP.read(buf, 48);                 // читаем
				if (buf[40] == 0) {  		            // некорректное время
					_send_pack = false;
					return 6;
				}
				_last_upd = millis();                   // запомнили время обновления                 		
				uint16_t a_ms = (uint32_t)((uint32_t)(buf[44] << 8 | buf[45]) << 16 | (buf[46] << 8 | buf[47])) / 4294967.296;// мс сервера		
				_last_upd -= (a_ms + _ping);      		// смещение для дальнейших расчётов
				_unix = (((uint32_t)buf[40] << 24) | ((uint32_t)buf[41] << 16) | ((uint32_t)buf[42] << 8) | buf[43]);  // 1900
				_unix -= 2208988800ul;                  // перевод в UNIX (1970)
				_send_pack = false;
				_tmr = millis();            			// сброс таймера
				return 0;
			}
		}
		return 7;
    }
	
	//обновить сейчас
	void updateNow() {
		_tmr = millis() - _prd;
	}
    
    // миллисекунд с последнего обновления (с 0 секунд unix)
    uint32_t msFromUpdate() {
        return millis() - _last_upd;
    }
    
    // unix время
    uint32_t unix() {
        return _unix + (msFromUpdate() / 1000ul);
    }

    // миллисекунды текущей секунды
    uint16_t ms() {
        return msFromUpdate() % 1000;
    }
    
    // получить секунды
    uint8_t second() {
        //updateTime();
        //return _s;
        return (unix() + _gmt * 3600ul) % 60;
    }
    
    // получить минуты
    uint8_t minute() {
        updateTime();
        return _m;
    }
    
    // получить часы
    uint8_t hour() {
        updateTime();
        return _h;
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
    
    // пересчёт unix во время и дату и буферизация
    void updateTime() {
        if (millis() - _prev_calc < 100) return;
        _prev_calc = millis();
        // http://howardhinnant.github.io/date_algorithms.html#civil_from_days
        uint32_t u = unix() + _gmt * 3600ul;
        _s = u % 60ul;
        u /= 60ul;
        _m = u % 60ul;
        u /= 60ul;
        _h = u % 24ul;
        u /= 24ul;
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
    
    // получить пинг сервера
    int16_t ping() {
        return _ping;
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
		7 - идёт обновление
    */
    uint8_t status() {
        return _stat;
    }
    
private:
    const char* _host = GN_DEFAULT_HOST;
    uint32_t _prev_calc = 0;
    uint32_t _last_upd = 0;
    uint32_t _tmr = 0;
    uint32_t _prd = 360 * 1000;
    uint32_t _unix = 0;
    int8_t _gmt = 0;
    int16_t _ping = 0;
    uint8_t _stat = 5;
	
	uint32_t rtt = 0;
	bool _send_pack = 0;
    
    uint8_t _day, _month, _dayw, _h, _m, _s;
    int16_t _year;
};
#endif