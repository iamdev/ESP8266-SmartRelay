//***********************************************************************************
// MIT License
// 
// Copyright (c) 2018 Kamon Singtong
//***********************************************************************************
// Owner : Kamon Singtong (MakeArduino.com)
// email : kamon.dev@hotmail.com
// fb : makearduino
//***********************************************************************************

#ifndef _WIFI_CONFIG_H
#define _WIFI_CONFIG_H
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include "ArduinoJson.h"

/*Define to store config in EEPROM, otherwise store in Internal Flase*/
#ifdef CONFIG_STORAGE_EEPROM
#include "eeprom.h"
#else
#include <EEPROM.h>
#endif
 
#define DEFAULT_AP_SSID ""
#define DEFAULT_AP_PASS ""
#define AP_DNS_PORT 53


static String getEspName(){
  char esp_name[10];
  sprintf(esp_name, "ESP_%06X", ESP.getChipId());
  return String(esp_name);
}

template <class T>
class AppSetting{
    public :
        bool init(uint16_t signature=0);
        bool load();
        void save(T config);
        void save();
        void format();
        void onSetDefault(void (*callback)(T &config));
        void onFormat(void (*callback)(void));
        T config;
    private:        
        void (*onSetDefaultCallback)(T &config);
        void (*onFormatCallback)(void);
        uint16_t _signature;
};

struct field_t{
    const char *name;
    const char *desc;
    const char *value;
};

class WebConfig{
    public :
        WebConfig();
        WebConfig(const char * ap_ssid,const char * ap_pass);
        ~WebConfig();
        void begin(bool enableCaptivePortal=true);        
        void addConfigField(const char* name,const char * desc,const char * value);
        void handleCallback(void (*handler) (JsonObject));
        void onFactoryReset(void (*handler)(void));
        void onClearConfig(void (*handler)(void));        
        bool run();
    private:
        void handleRoot(void);
        void handleConfig(void);
        void handleInfo();
        void handleFactoryReset();
        void handleClearConfig();
        bool isIp(String str);
        String toStringIp(IPAddress ip);
        ESP8266WebServer *server;
        DNSServer*dnsServer;
        void (*_handleCallback) (JsonObject) = NULL;
        void (*_factoryCallback)(void) = NULL;
        void (*_clearCallback)(void) = NULL;
        bool isCompleted = false;        
        bool isEnableCaptivePortal;
        char ap_ssid[32];
        char ap_pass[32];
        int configFieldCount=0;
        field_t configFields[30];
};

template <class T>
bool AppSetting<T>::init(uint16_t signature){
    _signature = signature;
#ifdef CONFIG_STORAGE_EEPROM    
    eeprom.begin();
#else
    EEPROM.begin(512);
#endif    
    return load();
}

template <class T>
void AppSetting<T>::onSetDefault(void (*callback)(T &config)){
    onSetDefaultCallback = callback;
}

template <class T>
bool AppSetting<T>::load(){
#ifdef CONFIG_STORAGE_EEPROM 
    uint16_t signature;
    eeprom.read(sizeof(T),&signature,2);
    if(signature == _signature){
        eeprom.read(0,&config,sizeof(T));
    }
    else
    {
        config = {};
        if(onSetDefaultCallback) onSetDefaultCallback(config);
    }
#else    
    uint16_t signature = (EEPROM.read(sizeof(T)) | EEPROM.read(sizeof(T)+1)<<8);
    if(signature == _signature){
        unsigned char data[sizeof(T)];
        for(int i=0;i<sizeof(T);i++){
            data[i] = EEPROM.read(i);
        }
        memcpy(&config,data,sizeof(T));
    }else{
        config = {};
        if(onSetDefaultCallback) onSetDefaultCallback(config);
    }
#endif    
}

template <class T>
void AppSetting<T>::save(){       
    save(config);
}

template <class T>
void AppSetting<T>::save(T cfg){       
#ifdef CONFIG_STORAGE_EEPROM 
    eeprom.write(0,&cfg,sizeof(T));
    eeprom.write(sizeof(T),&_signature,2);
#else    
  unsigned char data[sizeof(T)];
  memcpy(data,&cfg,sizeof(T));
  for(int i =0;i<sizeof(T);i++){
    EEPROM.write(i,data[i]);
  }
  EEPROM.write(sizeof(T),_signature&0xff);
  EEPROM.write(sizeof(T)+1,(_signature>>8)&0xff); 
  EEPROM.commit();  
#endif  
  config = cfg;
}
template <class T>
void AppSetting<T>::onFormat(void(*callback)(void)){
    onFormatCallback = callback;
}

template <class T>
void AppSetting<T>::format(){       
    if(onSetDefaultCallback) onSetDefaultCallback(config);
    save(config);
    if(onFormatCallback)onFormatCallback();
}

#endif
