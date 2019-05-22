#ifndef _START_STOP_TIMER_H
#define _START_STOP_TIMER_H
#include "rtc.h"

class StartStopTimer{
public:
    void onStart(void (*callback)(void)) { onStartCallback = callback;}
    void onStop(void (*callback)(void)) { onStopCallback = callback;}
    void enable(bool isEnable=true) {_isEnabled = isEnable;if(!isEnable)_state = false;}
    void disable() {_isEnabled = false;_state = false;}
    bool isEnabled() const  { return _isEnabled;}
    void setTimer(uint32_t start,uint32_t stop) {_start = start;_stop = stop; }
    void loop(){
        if(!_isEnabled)return;
        time_t tt = now();        
        DateTime dt(tt);
        DateTime dz(dt.year,dt.month,dt.day);
        uint32_t t = tt - (time_t)dz;
        //Serial.printf("current :%d, start %d, stop %d, state : %d ",t,_start,_stop,_state?0:1);
        if(onStartCallback && !_state && t >= _start && (t<_stop || _start > _stop)){
            _state = true;
            onStartCallback();
        }
        if(onStopCallback && _state && t >= _stop && (_stop >_start || t < _start)){
            _state = false;
            onStopCallback();
        }
    }
    
private:
    bool _isEnabled = false;
    bool _state = false;
    void (*onStartCallback)(void);
    void (*onStopCallback)(void);
    uint32_t _start;
    uint32_t _stop;
};
#endif