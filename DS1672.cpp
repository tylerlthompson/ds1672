//
// Created by Tyler Thompson on 8/8/19.
//

#include "DS1672.h"
#include <Arduino.h>

#define RTC_ADDRESS 0x68
#define SECONDS_FROM_1970_TO_2000 946684800

DS1672::DS1672() = default;
DS1672::~DS1672() = default;

// Utilities from JeeLabs/Ladyada

////////////////////////////////////////////////////////////////////////////////
// utility code, some of this could be exposed in the DateTime API if needed

// DS3231 is smart enough to know this, but keeping it for now so I don't have
// to rewrite their code. -ADW
static const uint8_t daysInMonth [] PROGMEM = { 31,28,31,30,31,30,31,31,30,31,30,31 };

// number of days since 2000/01/01, valid for 2001..2099
static uint16_t date2days(uint16_t y, uint8_t m, uint8_t d) {
    if (y >= 2000)
        y -= 2000;
    uint16_t days = d;
    for (uint8_t i = 1; i < m; ++i)
        days += pgm_read_byte(daysInMonth + i - 1);
    if (m > 2 && y % 4 == 0)
        ++days;
    return days + 365 * y + (y + 3) / 4 - 1;
}

static long time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s) {
    return ((days * 24L + h) * 60 + m) * 60 + s;
}

static uint8_t getWeekDay(uint16_t yy, uint16_t mm, uint16_t dd)
{
    //formula to get weekday number
    int rst =
            dd
            + ((153 * (mm + 12 * ((14 - mm) / 12) - 3) + 2) / 5)
            + (365 * (yy + 4800 - ((14 - mm) / 12)))
            + ((yy + 4800 - ((14 - mm) / 12)) / 4)
            - ((yy + 4800 - ((14 - mm) / 12)) / 100)
            + ((yy + 4800 - ((14 - mm) / 12)) / 400)
            - 32045;

    return (rst+1)%7 ;
}

/*****************************************
        Public Functions
 *****************************************/

/*******************************************************************************
 * TO GET ALL DATE/TIME INFORMATION AT ONCE AND AVOID THE CHANCE OF ROLLOVER
 * DateTime implementation spliced in here from Jean-Claude Wippler's (JeeLabs)
 * RTClib, as modified by Limor Fried (Ladyada); source code at:
 * https://github.com/adafruit/RTClib
 ******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
// DateTime implementation - ignores time zones and DST changes
// NOTE: also ignores leap seconds, see http://en.wikipedia.org/wiki/Leap_second

DateTime::DateTime (uint32_t t) {
    t -= SECONDS_FROM_1970_TO_2000;    // bring to 2000 timestamp from 1970

    ss = t % 60;
    t /= 60;
    mm = t % 60;
    t /= 60;
    hh = t % 24;

    // 12 hour calc
    if(hh > 12){
        hh_12h = hh - 12;
        pm=true;
    }
    else if (hh == 0)
    {
        hh_12h = 12;
        pm = false;
    }
    else {
        hh_12h = hh;
        pm = false;
    }

    uint16_t days = t / 24;
    uint8_t leap;
    for (yOff = 0; ; ++yOff) {
        leap = yOff % 4 == 0;
        if (days < 365 + leap)
            break;
        days -= 365 + leap;
    }
    for (m = 1; ; ++m) {
        uint8_t daysPerMonth = pgm_read_byte(daysInMonth + m - 1);
        if (leap && m == 2)
            ++daysPerMonth;
        if (days < daysPerMonth)
            break;
        days -= daysPerMonth;
    }
    d = days + 1;

    uint16_t year;
    year = yOff + 2000;
    dow = getWeekDay(year, m, d);
}



DateTime::DateTime (uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec) {
    dow = getWeekDay(year, month, day);

    if (year >= 2000)
        year -= 2000;
    yOff = year;
    m = month;
    d = day;
    hh = hour;
    mm = min;
    ss = sec;

    // 12 hour calc
    if(hh > 12){
        hh_12h = hh - 12;
        pm=true;
    }
    else {
        pm = false;
    }

}

uint32_t DateTime::secondstime() const {
    uint32_t t;
    uint16_t days = date2days(yOff, m, d);
    t = time2long(days, hh, mm, ss);
    return t;
}

uint32_t DateTime::unixtime() const {
    uint32_t t = secondstime();
    t += SECONDS_FROM_1970_TO_2000;  // seconds from 1970 to 2000
    return t;
}

const char* DateTime::am_pm() {
    if(pm) {
        return "PM";
    }
    else {
        return "AM";
    }
}


void DS1672::enable() {
    // set EOSC bit to 0
    set_reg(0x04, 0x00);
}

void DS1672::disable() {
    // set EOSC bit to 1
    set_reg(0x04, 0x80);
}

DateTime DS1672::get_time() {
    reg_read = 0;
    reg_read = reg_read | (uint32_t) get_reg(0x03) << 24;
    reg_read = reg_read | (uint32_t) get_reg(0x02) << 16;
    reg_read = reg_read | (uint32_t) get_reg(0x01) << 8;
    reg_read = reg_read | (uint32_t) get_reg(0x00);
    reg_read += SECONDS_FROM_1970_TO_2000;
    return {reg_read};
}

void DS1672::set_time(DateTime now) {
    reg_read = now.secondstime();
    byte t = 0;
    t = (byte) reg_read;
    set_reg(0x00, t);
    t = (byte) (reg_read >> 8);
    set_reg(0x01, t);
    t = (byte) (reg_read >> 16);
    set_reg(0x02, t);
    t = (byte) (reg_read >> 24);
    set_reg(0x03, t);
}

byte DS1672::get_reg(uint8_t address) {
    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(address);
    Wire.endTransmission();
    Wire.requestFrom(RTC_ADDRESS, 1);
    return Wire.read();
}

void DS1672::set_reg(uint8_t address, byte value) {
    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(address);
    Wire.write(value);
    Wire.endTransmission();
}

byte DS1672::decToBcd(byte val) {
// Convert normal decimal numbers to binary coded decimal
    return ( (val/10*16) + (val%10) );
}

byte DS1672::bcdToDec(byte val) {
// Convert binary coded decimal to normal decimal numbers
    return ( (val/16*10) + (val%16) );
}