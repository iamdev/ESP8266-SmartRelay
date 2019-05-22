#define RESET_WIFI_TIMEOUT  0
#define RESET_INTERVAL 0

#include <ESP8266WiFi.h>
#include "src/ntp.h"
#include "src/task.h"
#include "src/rtc.h"

void printWifiInfo(){
    Serial.println("==========================================================");
    Serial.println("WiFi Information");
    Serial.println("----------------------------------------------------------");
    WiFi.printDiag(Serial);
    Serial.printf ("Device IP           : %s\n", WiFi.localIP().toString().c_str());
    Serial.printf ("Subnet Mask         : %s\n", WiFi.subnetMask().toString().c_str());
    Serial.printf ("Gateway             : %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf ("DNS                 : %s\n", WiFi.dnsIP().toString().c_str());
    Serial.println("==========================================================");    
}

bool isRequestNtp(true);
void requestNtp(){
    isRequestNtp = true;
}

void task_wifi_status(task_t &tk){  
  static unsigned long resetTime=0;
  tk.interval = 1000;  
  DateTime dt(now());
  if(RESET_INTERVAL >0 && resetTime==0 && dt.year>2000){
    resetTime = ((long)dt)+(RESET_INTERVAL);
    Serial.print("Next resart time : ");
    printDateTime((time_t)resetTime);
  }

  if(RESET_INTERVAL>0 && resetTime>0 && now()>resetTime){    
    ESP.restart();
  }
  
  static wl_status_t wifi_status = WL_DISCONNECTED;
  static time_t connection_time = now();
  if(WiFi.status() == WL_CONNECTED){    
    if(wifi_status != WL_CONNECTED){
      printWifiInfo();
    }
    
    if(isRequestNtp){
      Serial.println("Get ntp time...");
      NtpTime ntp("3.th.pool.ntp.org");
      ntp.begin();
      time_t t = ntp.getTime();
      if(DateTime(t).year>2000){
        printDateTime(t);
        rtc.set(t);   
        isRequestNtp = false;
      } 
    }
    connection_time = now();
  }else{
    if(RESET_WIFI_TIMEOUT>0 && now()>(connection_time+RESET_WIFI_TIMEOUT)){
      ESP.restart();
    }
  }
  wifi_status = WiFi.status();
}