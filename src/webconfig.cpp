//***********************************************************************************
// MIT License
// 
// Copyright (c) 2018 Kamon Singtong
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//***********************************************************************************
// Owner : Kamon Singtong (MakeArduino.com)
// email : kamon.dev@hotmail.com
// fb : makearduino
//***********************************************************************************
#include "webconfig.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

#define DEBUG
#include "debug.h"
 
WebConfig::WebConfig(const char * ap_ssid,const char * ap_pass){
  strncpy(this->ap_ssid,ap_ssid,sizeof(this->ap_ssid));
  strncpy(this->ap_pass,ap_pass,sizeof(this->ap_pass));
  //configFields = new field_t();
  configFieldCount = 0;
}

WebConfig::WebConfig():WebConfig(DEFAULT_AP_SSID,DEFAULT_AP_PASS){

}

bool WebConfig::run(){
  if(isEnableCaptivePortal){
    this->dnsServer->processNextRequest();
  }
  this->server->handleClient();
  return !isCompleted;
}

void WebConfig::addConfigField(const char* name,const char * desc,const char * value){     
    configFields[configFieldCount++] = {name,desc,value};
}

void WebConfig::handleRoot(){  
  server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server->sendHeader("Pragma", "no-cache");
  server->sendHeader("Expires", "-1");

  String s = "<!DOCTYPE html>";
  s+="<html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>Smart Config</title>";
  s+="<style>body{font-family:Arial;font-size:14px;}[center]{justify-content: center;}x{display:flex;flex-direction: column;/*border:solid 1px red;*/}[g]{position:relative;margin:1em 0 0 0;max-width: 25em;border-radius : 0.5em;border : solid 1px #000;padding:1em;}[row]{flex-direction: row;}[g] [h]{padding:0 0.5em;background-color: #fff;position:absolute;font-weight:bold;height:1.5em;top:-0.5em;}[f]{flex-direction: row;}[f] [lb]{flex: 0 0 10em;margin:0.3em 0;}button{height:2em;width:6em;}</style>";
  s+="</head><body><form method='post' action='/config'>";
  int i=0;  
  while(i<configFieldCount){
    //Serial.println(configFields[i].desc);
    s+="<x g><x h>";
    if(configFields[i].name == NULL) s+=configFields[i++].desc;
    s+="</x><x gb>";     
    for(;i<configFieldCount && configFields[i].name != NULL;i++){  
      s+="<x f><x lb>";
      s+=configFields[i].desc;
      s+=" :</x><x tb><input name='";
      s+=configFields[i].name;
      s+="' value='";
      s+=configFields[i].value;
      s+="'></x></x>"; 
    }    
    s+="</x></x>";    
  }
  
  s+="<x g row center>";
  if(_factoryCallback) s+="<button type=button style='width:10em' onClick='window.location=\"/factoryreset\"'>Factory Reset</button>";
  if(_clearCallback) s+="<button type=button onClick='window.location=\"/clearconfig\"'>Clear</button>";
  
  s+="<button type=submit>Save</button>"
    "</x></form></body></html>";
  server->setContentLength(s.length());
  server->send(200, "text/html", s);
}

void WebConfig::handleConfig(){
  StaticJsonDocument<4000> doc;
  bool isResultHtml = false;
  if (server->args() > 1){
    String s = "{";
    for (int i = 0; i < server->args();i++){
      if(i>0)s+=",";
      s+= "\"";
      s+= server->argName(i);
      s+= "\":\"";
      s+= server->arg(i);
      s+= "\"";      
    }
    s+="}";    
    #ifdef DEBUG
    Serial.println(s.c_str());
    #endif
    DeserializationError error = deserializeJson(doc, s.c_str());
      if (error) {
      return server->send(500, "text/json", "{success:false}");
    }
    isResultHtml = true;
  }else{
    DeserializationError error = deserializeJson(doc, server->arg("plain"));
    if (error) {
      return server->send(500, "text/json", "{success:false}");
    }
  }
  JsonObject data = doc.as<JsonObject>();    
  _handleCallback(data);
  isCompleted = true;
  if(isResultHtml){
    String s = "<!DOCTYPE html>";
    s+="<html><head>";
    s+="<meta name='viewport' content='width=device-width, initial-scale=1'>";
    s+="<title>Device Configuration</title><body>Success</body></html>";
    server->send ( 200, "text/html", s.c_str());    
  }else{
    server->send ( 200, "text/json", "{success:true}" );    
  }
  delay(1000);
}

void WebConfig::handleCallback(void (*handler) (JsonObject)){
  _handleCallback = handler;
}

void WebConfig::onFactoryReset(void (*handler)(void)){
  _factoryCallback = handler;
}
void WebConfig::onClearConfig(void (*handler)(void)){
  _clearCallback = handler;
}

void WebConfig::handleInfo(){
  String s = "{\"macAddress\":\"";
  s+=WiFi.macAddress(); 
  s+="\",fields:[";
  for(int i=0;i<configFieldCount;i++){
    delay(100);
    if(i>0)s+=",";
    s+="{\"name\":\"";
    s+=configFields[i].name;
    s+="\",\"desc\":\"";
    s+=configFields[i].desc;
    s+="\",\"value\":\"";
    s+=configFields[i].value;
    s+="\"}";
  }
  s+="]}";
  delay(100);
  server->send ( 200, "text/json", s.c_str());
}

void WebConfig::handleFactoryReset(){
  if(_factoryCallback)_factoryCallback();
  server->sendHeader("Location", "/",true); //Redirect to our html web page 
  server->send(302, "text/plane",""); 
}
void WebConfig::handleClearConfig(){
  if(_clearCallback)_clearCallback();
  server->sendHeader("Location", "/",true); //Redirect to our html web page 
  server->send(302, "text/plane",""); 
}
void WebConfig::begin(bool enableCaptivePortal){
  isEnableCaptivePortal = enableCaptivePortal;  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid,ap_pass);
  WiFi.hostname(this->ap_ssid);
  IPAddress myIP = WiFi.softAPIP();

#ifdef DEBUG
  SerialDebug_printf("SoftAP SSID : %s, Pass : %s\n",ap_ssid,ap_pass);
  SerialDebug_printf("IP Address  : %s\n",myIP.toString().c_str());  
#endif
  
  server = new ESP8266WebServer(80);
  server->on("/", [this]{handleRoot();});
  server->on("/config",HTTP_POST, [this]{handleConfig();});
  server->on("/info", [this]{handleInfo();});
  server->on("/factoryreset", [this]{handleFactoryReset();});
  server->on("/clearconfig", [this]{handleClearConfig();});
  if(isEnableCaptivePortal){
    dnsServer = new DNSServer();
    dnsServer->start(AP_DNS_PORT, "*", myIP);
    server->onNotFound([this]{handleRoot();});
  }
  server->begin();
}

WebConfig::~WebConfig(){
  server->stop(); 
}
