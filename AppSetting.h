#ifndef APP_SETTING_H
#define APP_SETTING_H

/*Define to store config in EEPROM, otherwise store in Internal Flase*/
#define CONFIG_STORAGE_EEPROM

#include "src/webconfig.h"
#include <IPAddress.h>

typedef struct Config{
  char wifi_ssid[50];
  char wifi_pass[50];
  char ap_pass[50];
  IPAddress ip;
  IPAddress subnet;
  IPAddress gateway;  
  IPAddress dns;
  char blynk_server[50];
  int blynk_port = 80;
  char blynk_token[50]; 
}Config_t;

extern AppSetting<Config_t>appSetting;

void ipToCharArray(IPAddress ip,char*ipStr);
#define IP_TO_CHARARRAY(ip,ar) ipToCharArray(ip,ar)
void factory_reset();
void handle_WebConfig();
void printSetting();

/***************************************************************************/
/* Implement
/***************************************************************************/
void ipToCharArray(IPAddress ip,char*ipStr){
  if(ip){
    snprintf(ipStr,16,"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
  }else{
    String("0.0.0.0").toCharArray(ipStr,16);
  }
}

void handle_WebConfig(){
    Serial.printf("Enter AP Config Mode...\n");  
  for(int i=0;i<3;i++){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
  Config config = appSetting.config;
  WebConfig wc(getEspName().c_str(), config.ap_pass);
  wc.begin();
  wc.onFactoryReset([]{
    factory_reset();
  });
//  wc.onClearConfig([] {
//  });

  char ip[16], subnet[16], gateway[16], dns[16];
  
  IP_TO_CHARARRAY(config.ip,ip);
  IP_TO_CHARARRAY(config.subnet,subnet);
  IP_TO_CHARARRAY(config.gateway,gateway);
  IP_TO_CHARARRAY(config.dns,dns);
  
  wc.addConfigField(NULL, "WIFI Config", "");
  wc.addConfigField("wifi_ssid", "SSID", config.wifi_ssid);
  wc.addConfigField("wifi_pass", "Pass", config.wifi_pass);
  wc.addConfigField("ap_pass", "AP Pass", config.ap_pass);

  wc.addConfigField(NULL,"Network Address","");  
  wc.addConfigField("ip","Static IP",ip);
  wc.addConfigField("subnet","Subnet Mask",subnet);
  wc.addConfigField("gateway","Gateway",gateway);  
  wc.addConfigField("dns","DNS",dns);  

  String blynk_port = String(config.blynk_port,DEC);

  wc.addConfigField(NULL, "Blynk", "");
  wc.addConfigField("blynk_server", "Blynk Server", config.blynk_server);
  wc.addConfigField("blynk_port", "Port", blynk_port.c_str());
  wc.addConfigField("blynk_token", "Token", config.blynk_token);
  
  wc.handleCallback([](JsonObject doc) {
    Config config = appSetting.config;
    Serial.printf("Saving config...\n");
    Serial.printf("  wifi_ssid    : %s\n", (const char *)doc["wifi_ssid"]);
    Serial.printf("  wifi_pass    : %s\n", (const char *)doc["wifi_pass"]);
    Serial.printf("  ap_pass      : %s\n", (const char *)doc["ap_pass"]);
    
    Serial.printf("  ip           : %s\n", (const char *)doc["ip"]);
    Serial.printf("  subnet       : %s\n", (const char *)doc["subnet"]);
    Serial.printf("  gateway      : %s\n", (const char *)doc["gateway"]);
    Serial.printf("  dns          : %s\n", (const char *)doc["dns"]);

    Serial.printf("  blynk_server : %s\n", (const char *)doc["blynk_server"]);
    Serial.printf("  blynk_port   : %s\n", (const char *)doc["blynk_port"]);
    Serial.printf("  blynk_token  : %s\n", (const char *)doc["blynk_token"]);    
        
    strncpy(config.wifi_ssid, (const char *)doc["wifi_ssid"], sizeof(config.wifi_ssid));
    strncpy(config.wifi_pass, (const char *)doc["wifi_pass"], sizeof(config.wifi_pass));
    strncpy(config.ap_pass, (const char *)doc["ap_pass"], sizeof(config.ap_pass));

    config.ip.fromString((const char *)doc["ip"]);
    config.subnet.fromString((const char *)doc["subnet"]);
    config.gateway.fromString((const char *)doc["gateway"]);
    config.dns.fromString((const char *)doc["dns"]);

    strncpy(config.blynk_server,(const char *)doc["blynk_server"],sizeof(config.blynk_server));
    strncpy(config.blynk_token,(const char *)doc["blynk_token"],sizeof(config.blynk_token));
    config.blynk_port = atol((const char *)doc["blynk_port"]);

    appSetting.save(config);
  
    Serial.printf("Save config success.");
  });

  while (wc.run())yield();
  ESP.restart();
}

void factory_reset() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  appSetting.onSetDefault([](Config &config){
    strncpy(config.ap_pass, String("").c_str(), sizeof(config.ap_pass));
    strncpy(config.wifi_ssid, String("").c_str(), sizeof(config.wifi_ssid));
    strncpy(config.wifi_pass, String("").c_str(), sizeof(config.wifi_pass));
    strncpy(config.blynk_server, String("blynk-cloud.com").c_str(), sizeof(config.blynk_server));
    strncpy(config.blynk_token, String("").c_str(), sizeof(config.blynk_token));
    config.blynk_port = 80;  
    config.ip = {0,0,0,0};
    config.subnet = {255,255,255,0};
    config.gateway = {0,0,0,0};
    config.dns = {0,0,0,0};  
  });
  appSetting.format();
}

void printSetting(){
    Config config = appSetting.config;
    char ip[16], subnet[16], gateway[16], dns[16];
    IP_TO_CHARARRAY(config.ip,ip);
    IP_TO_CHARARRAY(config.subnet,subnet);
    IP_TO_CHARARRAY(config.gateway,gateway);
    IP_TO_CHARARRAY(config.dns,dns);
    
    Serial.println("==========================================================");
    Serial.println("App Settings");
    Serial.println("----------------------------------------------------------");
    Serial.printf("  wifi_ssid    : %s\n", config.wifi_ssid);
    Serial.printf("  wifi_pass    : %s\n", config.wifi_pass);
    Serial.printf("  ap_pass      : %s\n", config.ap_pass);    
    Serial.printf("  ip           : %s\n", ip);
    Serial.printf("  subnet       : %s\n", subnet);
    Serial.printf("  gateway      : %s\n", gateway);
    Serial.printf("  dns          : %s\n", dns);
    Serial.printf("  blynk_server : %s\n", config.blynk_server);
    Serial.printf("  blynk_port   : %d\n", config.blynk_port);
    Serial.printf("  blynk_token  : %s\n", config.blynk_token);    
    Serial.println("==========================================================");
}
#endif