#include "rtc.h"

//********************************************************************/
// Implement DateTime
//********************************************************************/
DateTime::DateTime (time_t t) {
    static int daysPerMonths[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int y = 0;
    int m =0;
    int d =0;
    int ss = t % 60;
    t /= 60;
    int mm = t % 60;
    t /= 60;
    int hh = t % 24;
    uint16_t days = t / 24;
    uint8_t leap;
    for (y = 0; ; ++y) {
        leap = (y+2) % 4 == 0;
        if (days < 365 + leap)
            break;
        days -= 365 + leap;
    }
    for (m = 1; ; ++m) {
        uint8_t daysPerMonth = daysPerMonths[m-1];
        if (leap && m == 2)
            ++daysPerMonth;
        if (days < daysPerMonth)
            break;
        days -= daysPerMonth;
    }
    d = days + 1; 
    year = y+1970;
    month = m;
    day = d;
    hour = hh;
    minute = mm;
    second = ss;
}

DateTime::DateTime (uint16_t y, uint8_t m, uint8_t d, uint8_t hh, uint8_t mm, uint8_t ss) {
    year = y;
    month = m;
    day = d;
    hour = hh;
    minute = mm;
    second = ss;
}

DateTime::DateTime (const char* datetime)
{
    uint8_t buf[6] = {1900,1,1,0,0,0};
    char str[32];
    strncpy(str,datetime,32);
    char del[] = ":/- ";
    char * tok= strtok(str,del);
    int i = 0;
    bool yearFirst = false;
    while(i<6 && tok){      
        if(i==0 && datetime[strlen(tok)] == ':')i=3;
        int v = atol(tok);            
        if(i==0 && v>99) yearFirst = true;
        if(v>99)v=v%2000;
        buf[i++] = v;
        tok = strtok(NULL,del);
    }

    if(yearFirst){
        this->year = buf[0]+2000;
        this->month = buf[1];
        this->day = buf[2];
    }else{
        this->year = buf[2]+2000;
        this->month = buf[1];
        this->day = buf[0];    
    }
    this->hour = buf[3];
    this->minute = buf[4];
    this->second = buf[5];

}

byte DateTime::dayOfWeek() {   // y > 1752, 1 <= m <= 12
  int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};  
  year -= month < 3;
  return ((year + year/4 - year/100 + year/400 + t[month-1] + day) % 7) + 1; // 01 - 07, 01 = Sunday
}

DateTime:: operator time_t(){
    static int monthdays[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
	if(year<1970) return 0;
    int yd = year-1970;
    long days = ((yd)*365) + ((yd+2)/4) + (day-1);
    for(int i = 0;i<month;i++) days+=monthdays[i];
    return (time_t) (((days * 24L + hour) * 60 + minute) * 60 + second);
}
//********************************************************************/
//********************************************************************/

#ifdef DS3231_TIME_BASED

#define SECONDS_FROM_1970_TO_2000   946684800
#define DS3231_I2C_ADDR             0x68
#define DS3231_TIME_CAL_ADDR        0x00
#define DS3231_TEMPERATURE_ADDR     0x11
#define DS3231_CONTROL_ADDR         0x0E
#define DS3231_STATUS_ADDR          0x0F
#define DS3231_ALARM1_ADDR          0x07
#define DS3231_ALARM2_ADDR          0x0B
// control register bits
#define DS3231_A1IE     0x1
#define DS3231_A2IE     0x2
#define DS3231_INTCN    0x4

//********************************************************************/
// Implement SimpleRTC
//********************************************************************/
static uint8_t dectobcd(const uint8_t val)
{
    return ((val / 10 * 16) + (val % 10));
}

static uint8_t bcdtodec(const uint8_t val)
{
    return ((val / 16 * 10) + (val % 16));
}

static uint8_t inp2toi(char *cmd, const uint16_t seek)
{
    uint8_t rv;
    rv = (cmd[seek] - 48) * 10 + cmd[seek + 1] - 48;
    return rv;
}

SimpleRTC::SimpleRTC(TwoWire *wire)
    :_wire(wire)
{    
}

SimpleRTC::SimpleRTC()
    :SimpleRTC(&Wire)
{
}

void SimpleRTC::set_addr(const uint8_t addr, const uint8_t val)
{
    _wire->beginTransmission(DS3231_I2C_ADDR);
    _wire->write(addr);
    _wire->write(val);
    _wire->endTransmission();
}

uint8_t SimpleRTC::get_addr(const uint8_t addr)
{
    uint8_t rv;

    _wire->beginTransmission(DS3231_I2C_ADDR);
    _wire->write(addr);
    _wire->endTransmission();

    _wire->requestFrom(DS3231_I2C_ADDR, 1);
    rv = _wire->read();

    return rv;
}

void SimpleRTC::set_creg(const uint8_t val)
{
    set_addr(DS3231_CONTROL_ADDR, val);
}

void SimpleRTC::set_sreg(const uint8_t val)
{
    set_addr(DS3231_STATUS_ADDR, val);
}

uint8_t SimpleRTC::get_sreg(void)
{
    uint8_t rv;
    rv = get_addr(DS3231_STATUS_ADDR);
    return rv;
}

float SimpleRTC::getTemperature()
{
    float rv;
    uint8_t temp_msb, temp_lsb;
    int8_t nint;

    _wire->beginTransmission(DS3231_I2C_ADDR);
    _wire->write(DS3231_TEMPERATURE_ADDR);
    _wire->endTransmission();

    _wire->requestFrom(DS3231_I2C_ADDR, 2);
    temp_msb = _wire->read();
    temp_lsb = _wire->read() >> 6;

    if ((temp_msb & 0x80) != 0)
        nint = temp_msb | ~((1 << 8) - 1);      // if negative get two's complement
    else
        nint = temp_msb;
    rv = 0.25 * temp_lsb + nint;
    return rv;
}

DateTime SimpleRTC::now()
{
    uint8_t TimeDate[7];        //second,minute,hour,dow,day,month,year
    uint8_t century = 0;
    uint8_t i, n;
    uint16_t year_full;
    _wire->beginTransmission(DS3231_I2C_ADDR);
    _wire->write(DS3231_TIME_CAL_ADDR);
    _wire->endTransmission();

    _wire->requestFrom(DS3231_I2C_ADDR, 7);
    for (i = 0; i <= 6; i++) {
        n = _wire->read();
        if (i == 5) {
            TimeDate[5] = bcdtodec(n & 0x1F);
            century = (n & 0x80) >> 7;
        } else
            TimeDate[i] = bcdtodec(n);
    }
	//Serial.printf("Century = %d, Year = %d\n",century,TimeDate[6]);
	
    if (century == 1) {
        year_full = 2000 + TimeDate[6];
    } else {
		year_full = 1900 + TimeDate[6];
    }
    return DateTime(year_full,TimeDate[5],TimeDate[4],TimeDate[2],TimeDate[1],TimeDate[0]);
}

void SimpleRTC::set(DateTime t)
{ 
    uint8_t i, century; 
    uint8_t short_year;
    if (t.year >= 2000) {
        century = 0x80;
        short_year = t.year - 2000;
    } else {
        century = 0;
        short_year = t.year - 1900;
    }
    uint8_t TimeDate[7] = { t.second, t.minute, t.hour, t.dayOfWeek(), t.day, t.month,short_year };
    _wire->beginTransmission(DS3231_I2C_ADDR);
    _wire->write(DS3231_TIME_CAL_ADDR);
    for (i = 0; i <= 6; i++) {
        TimeDate[i] = dectobcd(TimeDate[i]);
        if (i == 5)
            TimeDate[5] += century;
        _wire->write(TimeDate[i]);                
    }
    _wire->endTransmission();
    setLocal(t);
}

void SimpleRTC::setLocal()
{
    setLocal(this->now());
}

#else
SimpleRTC::SimpleRTC(){}
void SimpleRTC::set(DateTime t)
{
    setLocal(t);
}
DateTime SimpleRTC::now(){
    return localTime();
}
#endif


void SimpleRTC::set(time_t t)
{
    set(DateTime(t));
} 
void SimpleRTC::setLocal(DateTime t){
  setLocal((time_t)t);
}
void SimpleRTC::setLocal(time_t t){
//  Serial.print("Set local (unixtime):");
//  Serial.println(t);
  timeoffset = t - (millis()/1000);
}

struct DateTime SimpleRTC::localTime(){
  return DateTime(timeoffset + (millis()/1000));
}



//********************************************************************/
//********************************************************************/


time_t now(){
    return (time_t)rtc.localTime(); 
}

int year(){return DateTime(now()).year;}
int month(){return DateTime(now()).month;}
int day(){return DateTime(now()).day;}
int hour(){return DateTime(now()).hour;}
int minute(){return DateTime(now()).minute;}
int second(){return DateTime(now()).second;}

void printDateTime(time_t tt,char*buf){
    printDateTime(DateTime(tt),buf);
}
void printDateTime(time_t tt,bool newline){
    printDateTime(DateTime(tt),newline);
}
void printDateTime(DateTime dt,char*buf){
    snprintf(buf,32,"%02u/%02u/%04u %02u:%02u:%02u",dt.day,dt.month,dt.year,dt.hour,dt.minute,dt.second);      
}
void printDateTime(DateTime dt,bool newline){
    char str[32];
    printDateTime(dt,str);    
    Serial.print(str);
    if(newline) Serial.println();
}
