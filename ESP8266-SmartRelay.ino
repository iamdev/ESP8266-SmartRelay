/**********************************************************************/
//  Sketch : ESP8266 SmartRelay
//  Board : WeMos D1 mini   
//  Shield : SmartRelay4CH V2
//  Product Url : http://www.makearduino.com/product/61/esp8266-wifi-smartrelay-4-channel-for-wemos-d1-mini
//  Facebook :  https://www.facebook.com/commerce/products/2275475742511114/
/**********************************************************************/
#define DEBUG
#define SERIAL_BAUD_RATE 115200
#define BLYNK_PRINT Serial
#define APP_DATA_ADDRESS 1024
#define USE_LCD_1602
#define USE_DHT22
#define BLYNK_MSG_LIMIT 250
#define PCF8574_I2C_ADDR 0x20 /*0x38|0x20,0x39|0x21*/

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

#include <ESP8266WiFi.h>
#define PHY_MODE WIFI_PHY_MODE_11G

/**********************************************************************/
/* Include Libraries                                                  */
/**********************************************************************/
#include "src/debug.h"
#include "src/task.h"
#include "src/button.h"
#include "src/rtc.h"
#include "src/eeprom.h"
#include "src/PCF8574.h"
#include "src/mv-ring-buffer.h"
#include "src/StartStopTimer.h"
#include "AppSetting.h"
#include <user_interface.h>
#include <BlynkSimpleEsp8266.h>

/**********************************************************************/
/* Type Difinition                                                    */
/**********************************************************************/
typedef struct {
    struct timer_info_t {
      uint32_t start;
      uint32_t stop;
      uint8_t isEnabled;
    } timer [4];
}AppData_t;
AppData_t appData = {};

Button btn(0);
AppSetting<Config_t> appSetting;
Config_t config;
SimpleRTC rtc;
PCF8574 ex(PCF8574_I2C_ADDR);

float temperature, humidity;
const int a0_buffer_size = 50;
double a0_buffer[a0_buffer_size];
MVRingBuffer<double> rba0(a0_buffer,a0_buffer_size);
const int exp_input_pin[] = {4,5,6,7};
const int exp_output_pin[] = {0,1,2,3};

StartStopTimer sstm[4];
void onInputChanged();
void onTimerStart(int n);
void onTimerStop(int n);

void blynkPushData();

#include "task_wifi_status.h"
void setup() {
  pinMode(LED_BUILTIN,OUTPUT);
  Serial.begin(SERIAL_BAUD_RATE); 
  Serial.println();
  Wire.begin(SDA,SCL);
  rtc.setLocal();
  Serial.print("RTC Date/Time : ");
  printDateTime(now());
  

  #ifdef USE_LCD_1602 
  setup_lcd();
  #endif

  #ifdef USE_DHT22
  setup_dht();
  #endif
 
  /******************************************/
  /* Load application setting from Storage  */
  /******************************************/ 
  appSetting.init(0x1b30);
  appSetting.onFormat([]{
    appData = {};
    eeprom.write(APP_DATA_ADDRESS,&appData,sizeof(AppData_t));
  });
  eeprom.read(APP_DATA_ADDRESS,&appData,sizeof(AppData_t));
  if(appSetting.config.wifi_ssid[0] == 0) handle_WebConfig();  
  config = appSetting.config;
  #ifdef DEBUG
  printSetting();
  #endif
  /******************************************/ 
  
  setup_io_exp();  
  
  /******************************************/
  /* Setup WIFI Station                     */
  /******************************************/ 
  if (config.ip[0] > 0 && config.dns[0] > 0) {
    Serial.println("Config WIFI for static Ip and fixed DSN");
    WiFi.config(config.ip, config.gateway, config.subnet, config.dns);
  } else if (config.ip[0] > 0) {
    Serial.println("Config WIFI for static Ip");
    WiFi.config(config.ip, config.gateway, config.subnet);
  }   
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.disconnect(true);  
  WiFi.begin(config.wifi_ssid, config.wifi_pass);
  wifi_set_sleep_type(NONE_SLEEP_T);
  /******************************************/ 
  
  /******************************************/
  /* Setup Blynk Interface                  */
  /******************************************/ 
  Blynk.config(config.blynk_token,config.blynk_server, config.blynk_port);
  Task.create([]{if(WiFi.status() == WL_CONNECTED)Blynk.run();},1);    
  /******************************************/ 

  /******************************************/
  /* Setup Button                           */
  /******************************************/ 
  btn.begin(LOW,200,50);
  btn.onPressed([]{
    handle_WebConfig();
  });
  /*monitor button pressed*/
  Task.create([]{
    btn.loop();
  },10);  
  /******************************************/
  
    /*LED Status*/
  Task.create([](task_t &tk) {
    static unsigned long ms=0;
    static bool state (false);
    int t1 = 50;
    int t2 = 1000;
    if(WiFi.status() != WL_CONNECTED){
      t1 = 500;
      t2 = 1000;
    }else if(!Blynk.connected()){
      t1 = 50;
      t2 = 2000;      
    }
    if(ms==0){
      digitalWrite(LED_BUILTIN,LOW);
      state = true;
      ms = millis();            
    }else if(millis()-ms>=t2){
      ms = 0;
    }else if(state && millis()-ms>=t1){
      digitalWrite(LED_BUILTIN,HIGH);
      state = false;
    }
  },50);
    
  /*Read analog and calculate moving average*/
  Task.create([]{
    double a = 4*(analogRead(A0))/1024.0;
    rba0.add(a);
  },10);
  
  /*Checking Wifi Status*/
  Task.create(task_wifi_status,100);
  Task.create(blynkPushData,1000);
  Serial.println("Setup Completed."); 
}

void loop() {
  Task.loop();  
}


/****************************************************************************/
/* Blynk Event Handler                                                      */
/****************************************************************************/
#define BLYNK_OUTPUT_SWITCH(n) BLYNK_WRITE(V ## n) { \
  ex.write(n,!param.asInt()); \
  Blynk.virtualWrite(V20+n,param.asInt()?255:0); \
}

#define BLYNK_OUTPUT_PUSH(n) BLYNK_WRITE(V ## n) { \
  if(!param.asInt()){ \
    int x = n-10; \
    Blynk.virtualWrite(V20+x, ex.toggle(x)?0:255); \
  } \
}

void setTimer(int n,TimeInputParam t){
  BlynkTime s = t.getStart();
  BlynkTime e = t.getStop(); 
  Serial.printf("Timer CH %d  From %02u:%02u to %02u:%02u\n",n,s.hour(),s.minute(),e.hour(),e.minute());
  appData.timer[n].start = s.hour()*3600+s.minute()*60+s.second();
  appData.timer[n].stop = e.hour()*3600+e.minute()*60+e.second(),
  appData.timer[n].isEnabled = sstm[n].isEnabled()?1:0;
     
  eeprom.write(APP_DATA_ADDRESS,&appData,sizeof(appData));
  sstm[n].setTimer(appData.timer[n].start,appData.timer[n].stop);
}

#define BLYNK_OUTPUT_TIMER(n) BLYNK_WRITE(V ## n) {\
  int x = n-30; \
  TimeInputParam t(param);\
  setTimer(x,t);\
}

#define BLYNK_OUTPUT_TIMER_ENABLED(n) BLYNK_WRITE(V ## n) {\
  int x = n-34; \
  appData.timer[x].isEnabled = param.asInt(); \
  eeprom.write(APP_DATA_ADDRESS,&appData,sizeof(appData)); \
  sstm[x].enable(param.asInt()); \
}

BLYNK_OUTPUT_SWITCH(0);
BLYNK_OUTPUT_SWITCH(1);
BLYNK_OUTPUT_SWITCH(2);
BLYNK_OUTPUT_SWITCH(3);
BLYNK_OUTPUT_PUSH(10);
BLYNK_OUTPUT_PUSH(11);
BLYNK_OUTPUT_PUSH(12);
BLYNK_OUTPUT_PUSH(13);

BLYNK_OUTPUT_TIMER(30);
BLYNK_OUTPUT_TIMER(31);
BLYNK_OUTPUT_TIMER(32);
BLYNK_OUTPUT_TIMER(33);

BLYNK_OUTPUT_TIMER_ENABLED(34);
BLYNK_OUTPUT_TIMER_ENABLED(35);
BLYNK_OUTPUT_TIMER_ENABLED(36);
BLYNK_OUTPUT_TIMER_ENABLED(37);


BLYNK_READ(V4) //Blynk app has something on V5
{    
  //Blynk.virtualWrite(V24, ex.readAnalog()/1000.0); //sending to Blynk
  Blynk.virtualWrite(V4, rba0.average()); //sending to Blynk
}

unsigned long last_get_temp;
unsigned long last_get_humi;
unsigned long last_get_rssi;

BLYNK_READ(V5){  
  Blynk.virtualWrite(V5, WiFi.RSSI()); //sending to Blynk
  last_get_rssi  = millis();
}

BLYNK_READ(V6){  
  Blynk.virtualWrite(V6,temperature ); //sending to Blynk
  last_get_temp = millis();
}

BLYNK_READ(V7){  
  Blynk.virtualWrite(V7, humidity); //sending to Blynk
  last_get_humi = millis();
}


BLYNK_WRITE(V8){
  if (!param.asInt()){
      handle_WebConfig();
  }
}

BLYNK_CONNECTED() {
  //Blynk.syncAll();    
  Serial.println("Blynk Connected!");
}

void blynkPushData(){
  if(millis()-last_get_rssi>1000)Blynk.virtualWrite(V5, WiFi.RSSI()); //sending to Blynk
  if(millis()-last_get_temp>1000)Blynk.virtualWrite(V6, temperature); //sending to Blynk
  if(millis()-last_get_humi>1000)Blynk.virtualWrite(V7, humidity); //sending to Blynk
}
/****************************************************************************/
