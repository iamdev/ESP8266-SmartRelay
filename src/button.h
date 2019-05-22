#ifndef _Button_H
#define _Button_H
#include "Arduino.h"

class Button{
  public :
    Button(uint8_t pin):_pin(pin){}
    void begin();
    void begin(int mode,int threshold = 100,int sensitive = 20);
    void loop();
    void setThreshold(int);
    void setSensitive(int);
    void onPressed(void (* callback)(int));
    void onPressed(void (* callback)());
    void onDown(void (* callback)(void));
    bool isDown() const {return _isDown;}
    void setActiveMode(int mode);
    int duration() const {return _ts>0?millis()-_ts:0;}
  private:
    uint8_t _pin;
    uint32_t _ts;
    int activeMode;
    bool _isDown = false;
    void (*_onPressedCallback)(int);
    void (*_onDownCallback)(void);
    void (*_onReleaseCallback)(void);
    int _threshold=100;
    int _sensitive=20;
};

void Button::loop(){  
  #if defined(ARDUINO_ARCH_ESP8266)    
  if(activeMode == LOW) {
    pinMode(_pin,OUTPUT);    
    digitalWrite(_pin,HIGH);
    pinMode(_pin,INPUT);
  }  
  #endif
  
  int b = digitalRead(_pin);
  if(b == activeMode){
    if(_ts==0) {
      _ts = millis();
    }else{
      int ms = millis()-_ts;
      if(ms>_sensitive && !_isDown){
        _isDown = true;
        if(_onDownCallback)_onDownCallback();  
      }
    }
  }else{   
	if(_ts>0){
		int ms = millis()-_ts;
		if(ms>_threshold){          
		  if(_onReleaseCallback)_onReleaseCallback();
		  if(_onPressedCallback)_onPressedCallback(ms);       
		}
	}
    _isDown = false;
    _ts = 0;
  }

  /*
  if(_ts==0 && b == activeMode){
    _ts = millis();
    if(_onDownCallback)_onDownCallback();
  }else
  if(_ts>_threshold && b !=activeMode){    
    int ms = millis()-_ts;
    if(_onReleaseCallback)_onReleaseCallback();
    if(_onPressedCallback)_onPressedCallback(ms);
    _ts = 0;
  }*/
}

void Button::begin(){    
  pinMode(_pin,OUTPUT);
  digitalWrite(_pin,HIGH);
  pinMode(_pin,INPUT);
  setActiveMode(!digitalRead(_pin)?HIGH:LOW);
}

void Button::begin(int mode,int threshold,int sensitive){    
  setActiveMode(mode);
  setThreshold(threshold);
  setSensitive(sensitive);
}

void Button::onPressed(void (* callback)(int)){
  _onPressedCallback = callback;
}

void Button::onPressed(void (* callback)()){
  _onReleaseCallback = callback;
}

void Button::onDown(void (* callback)(void)){
  _onDownCallback = callback;
}

void Button::setActiveMode(int mode){
  if(mode == LOW){
    pinMode(_pin,INPUT_PULLUP);
  }else{
    #ifdef INPUT_PULLDOWN
    pinMode(_pin,INPUT_PULLDOWN);
    #else
    pinMode(_pin,INPUT);
    #endif
  }
  _ts = 0;
  activeMode = mode;
}

void Button::setThreshold(int threshold){
  _threshold = threshold;
}

void Button::setSensitive(int sensitive){
  _sensitive = sensitive;
}

#endif
