//
// Created by Tyler Thompson on 8/8/19.
//

#ifndef DS1672_h
#define DS1672_h

#include <Wire.h>
#include <Arduino.h>

// DateTime (get everything at once) from JeeLabs / Adafruit
// Simple general-purpose date/time class (no TZ / DST / leap second handling!)
class DateTime {
    public:
        DateTime (uint32_t t =0);
        DateTime (uint16_t year, uint8_t month, uint8_t day, uint8_t hour =0, uint8_t min =0, uint8_t sec =0);
        uint16_t year() const       { return 2000 + yOff; }
        uint8_t month() const       { return m; }
        uint8_t day() const         { return d; }
        uint8_t hour_12h() const    { return hh_12h; }
        uint8_t hour() const        { return hh; }
        uint8_t minute() const      { return mm; }
        uint8_t second() const      { return ss; }
        uint8_t dayOfWeek() const  { return dow; }
        uint32_t secondstime() const; // 32-bit times as seconds since 1/1/2000
        uint32_t unixtime() const;
        const char* am_pm();


    protected:
        uint8_t yOff, m, d, hh, mm, ss, hh_12h, dow;
        bool pm;
};

class DS1672 {
    public:
        DS1672();
        void enable();
        void disable();
        DateTime get_time();
        void set_time(DateTime now);
    private:
        byte get_reg(uint8_t address);
        void set_reg(uint8_t address, byte value);
        byte decToBcd(byte val);
        byte bcdToDec(byte val);
        uint32_t reg_read;
};

#endif //DS1672_h