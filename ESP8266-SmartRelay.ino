/**********************************************************************/
//  Sketch : ESP8266 SmartRelay
//  Board : ESP8266/WeMos D1 mini   
//  Shield : SmartRelay4CH V2.1
//  Product Url : http://www.makearduino.com/product/61/esp8266-wifi-smartrelay-4-channel-for-wemos-d1-mini
//  Facebook :  https://www.facebook.com/commerce/products/2275475742511114/
/**********************************************************************/
//  Prerequisite:
//  1. Library "Blynk" Install from Library Manager
//  2. LCD 1602 I2C (Optional)
//  3. DHT22 Temp/Humidity Sensor (Optinal)
//  4. Blynk App (optional - need 5800 energy point) 
//     - User Manual : https://github.com/iamdev/ESP8266-SmartRelay/blob/master/documents/manual.pdf
/**********************************************************************/
#define DEBUG
#define SERIAL_BAUD_RATE 115200
#define BLYNK_PRINT Serial
#define APP_DATA_ADDRESS 1024
#define USE_LCD_1602
#define USE_DHT22
#define BLYNK_MSG_LIMIT 250
#define PCF8574_I2C_ADDR 0x20 /*0x38|0x20,0x39|0x21*/
#define USE_SERIAL_COMMAND 

#ifdef USE_LCD_1602 
#include "src/LiquidCrystal_I2C.h"
LiquidCrystal_I2C lcd;
#endif

#ifdef USE_SERIAL_COMMAND
#include "src/SerialCommand.h"
SerialCommand cmd(&Serial);
#endif

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

EasyEEPROM eeprom;
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
bool eeprom_status = false;
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

#ifdef USE_LCD_1602 
  setup_lcd();
#endif

#ifndef CONFIG_STORAGE_EEPROM
  eeprom.begin();
  //Test EEPROM
  if(eeprom.read(0x00)!=0x3A){
    Serial.println("Write 0x3A to EERPOM address 0x00");  
    eeprom.write(0,0x3A);
  }
  uint8_t _test = eeprom.read(0x00);
  Serial.printf("Read EEPROM address 0x00 : %02X .... ",_test);  
  if(eeprom.read(0x00)==0x3A){
    Serial.println("Pass!");
    if(LCDStatus()){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("TEST EEPROM OK!");   
      delay(1000);
    }
    eeprom_status = true;
  }else{
    Serial.println("Fail!");
        
    if(LCDStatus()){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("EEPROM Fail!!!");        
      delay(10000);
    }
  }
#endif  
  
  rtc.setLocal();
  Serial.print("RTC Date/Time : ");
  printDateTime(now());
  Serial.println();

  #ifdef USE_DHT22
  setup_dht();
  #endif
 
  /******************************************/
  /* Load application setting from Storage  */
  /******************************************/ 
  appSetting.init(0x1b30);
  appSetting.onFormat([]{
    appData = {};
    if(eeprom_status)eeprom.write(APP_DATA_ADDRESS,&appData,sizeof(AppData_t));
  });
  if(eeprom_status){
    eeprom.read(APP_DATA_ADDRESS,&appData,sizeof(AppData_t));
  }
  if(appSetting.config.wifi_ssid[0] == 0) 
  {
    #ifdef USE_LCD_1602
    displayConfigMode();
    #endif
    handle_WebConfig();  
  }
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
    #ifdef USE_LCD_1602
    displayConfigMode();
    #endif
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
    double a = 5.2*(analogRead(A0))/1024.0;
    rba0.add(a);
  },10);

  #ifdef USE_SERIAL_COMMAND
  cmd.registerCommand("ON",[](const char * arg){
    int n = atoi(arg);
    if(n>=1 && n<=4){
      n--;
      ex.write(n,0);      
    }
  });
  cmd.registerCommand("OFF",[](const char * arg){
    int n = atoi(arg);
    if(n>=1 && n<=4){
      n--;
      ex.write(n,1);      
    }
  });
  Task.create([]{cmd.read();},100);
  #endif
  
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
  if(eeprom_status)eeprom.write(APP_DATA_ADDRESS,&appData,sizeof(appData));
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
  if(eeprom_status)eeprom.write(APP_DATA_ADDRESS,&appData,sizeof(appData)); \ 
  sstm[x].enable(param.asInt()); \
}
BLYNK_OUTPUT_TIMER(30);
BLYNK_OUTPUT_TIMER(31);
BLYNK_OUTPUT_TIMER(32);
BLYNK_OUTPUT_TIMER(33);
BLYNK_OUTPUT_TIMER_ENABLED(34);
BLYNK_OUTPUT_TIMER_ENABLED(35);
BLYNK_OUTPUT_TIMER_ENABLED(36);
BLYNK_OUTPUT_TIMER_ENABLED(37);



BLYNK_OUTPUT_SWITCH(0);
BLYNK_OUTPUT_SWITCH(1);
BLYNK_OUTPUT_SWITCH(2);
BLYNK_OUTPUT_SWITCH(3);
BLYNK_OUTPUT_PUSH(10);
BLYNK_OUTPUT_PUSH(11);
BLYNK_OUTPUT_PUSH(12);
BLYNK_OUTPUT_PUSH(13);



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

const uint8_t digitalPinMap[] ={16,5,4,0,2,14,12,13,15};

#define BLYNK_DIGITAL_READ(n) BLYNK_READ(V ## n) {\
    int p = 60-n;\
    if(p>=0 && p<=8)Blynk.virtualWrite(V60+p,digitalRead(digitalPinMap[p])?0:255);\
}

#define BLYNK_DIGITAL_WRITE(n) BLYNK_WRITE(V ## n) {\
    int p = 70-n;\
    if(p>=0 && p<=8)digitalWrite(digitalPinMap[p],param.asInt());\
}

BLYNK_DIGITAL_READ(60);//Write D0 (GPIO16)
BLYNK_DIGITAL_READ(61);//Write D1 (GPIO5)
BLYNK_DIGITAL_READ(62);//Write D2 (GPIO4)
BLYNK_DIGITAL_READ(63);//Write D3 (GPIO0)
BLYNK_DIGITAL_READ(64);//Write D4 (GPIO2)
BLYNK_DIGITAL_READ(65);//Write D5 (GPIO14)
BLYNK_DIGITAL_READ(66);//Write D6 (GPIO12)
BLYNK_DIGITAL_READ(67);//Write D7 (GPIO13)
BLYNK_DIGITAL_READ(68);//Write D8 (GPIO15)
BLYNK_DIGITAL_WRITE(70);//Read D0 (GPIO16)
BLYNK_DIGITAL_WRITE(71);//Read D1 (GPIO5)
BLYNK_DIGITAL_WRITE(72);//Read D2 (GPIO4)
BLYNK_DIGITAL_WRITE(73);//Read D3 (GPIO0)
BLYNK_DIGITAL_WRITE(74);//Read D4 (GPIO2)
BLYNK_DIGITAL_WRITE(75);//Read D5 (GPIO14)
BLYNK_DIGITAL_WRITE(76);//Read D6 (GPIO12)
BLYNK_DIGITAL_WRITE(77);//Read D7 (GPIO13)
BLYNK_DIGITAL_WRITE(78);//Read D8 (GPIO15)

BLYNK_WRITE(V99){
  if (!param.asInt()){
      #ifdef USE_LCD_1602
      displayConfigMode();
      #endif
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
