#ifndef _SIMPLE_RTC_H
#define _SIMPLE_RTC_H

#ifndef LOCAL_TIME_BASED
#define DS3231_TIME_BASED 
#endif

#include <Arduino.h>
#ifdef DS3231_TIME_BASED
#include <Wire.h>
#endif


class  DateTime {
  public:
      DateTime (time_t t);
      DateTime (uint16_t year, uint8_t month, uint8_t day, uint8_t hour =0, uint8_t min =0, uint8_t sec =0);
      DateTime (const char* datetime);
      uint16_t year;
      uint8_t month;
      uint8_t day;
      uint8_t hour;
      uint8_t minute;
      uint8_t second;
      uint8_t dayOfWeek();
      operator time_t();  	      
};

class SimpleRTC{
  public:
    
    #ifdef DS3231_TIME_BASED
    SimpleRTC(TwoWire *wire);
    float getTemperature();    
    #endif
    
    SimpleRTC();
    void begin();
    void setTimeZone(int timezone=0);
    void set(DateTime t);
    void set(time_t t);
    void setLocal(DateTime t);
    void setLocal(time_t t);  
    struct DateTime now();
    struct DateTime localTime();
    void setLocal();
    
  private:
    void set_addr(const uint8_t addr, const uint8_t val);
    uint8_t get_addr(const uint8_t addr);
    void set_creg(const uint8_t val);
    void set_sreg(const uint8_t val);
    uint8_t get_sreg(void);
    time_t timeoffset;
    int timezone = 0;
    uint32_t last_ms=0;   
    #ifdef DS3231_TIME_BASED
    TwoWire *_wire;
    #endif
};


void printDateTime(Stream*,time_t tt, bool newline=true);
void printDateTime(time_t tt,char*buf);
void printDateTime(Stream*,DateTime,bool newline=true);
void printDateTime(DateTime dt,char*buf);
void printDateTime(time_t tt);
void printDateTime(DateTime dt);
extern SimpleRTC rtc;

extern time_t now();
extern int year();
extern int month();
extern int day();
extern int hour();
extern int minute();
extern int second();
#endif /*_SIMPLE_RTC_H*/

