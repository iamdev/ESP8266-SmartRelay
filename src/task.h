//***********************************************************************************
// Library Name : SimpleTask for Arduino
// Architecture : Support All (AVR/ESP8266/STM32)
// Version : 1.0
// Owner : Kamon Singtong (MakeArduino.com)
// Web Site : https://www.makearduino.com
// facebook : /makearduino 
//***********************************************************************************
#ifndef _SIMPLE_TASK_H
#define _SIMPLE_TASK_H
#include <Arduino.h>

#ifndef MAX_TASK
#define MAX_TASK 16
#endif

#ifndef TASK_OVERFLOW_BIT
#define TASK_OVERFLOW_BIT      24
#endif

#define TASK_OVERFLOW_BITMASK    ((1<<(TASK_OVERFLOW_BIT))-1)

typedef struct task_t{
    void (*callback)(task_t&);
    void (*callback2)(void);    
    unsigned long interval;
    bool enabled;
    unsigned long timestamp;
    unsigned long next;
    unsigned long prev;
    unsigned long timeoffset;
    unsigned long offset;
};

class SimpleTask{
public : 
    SimpleTask();
    task_t* create(void (*callback)(task_t&),int interval);
    task_t* create(void (*callback)(void),int interval);
    void loop(unsigned long t);
    void loop();
protected:
    struct task_t tasks[MAX_TASK];
    int taskCount = 0;
    const unsigned long overflow_bitmask = TASK_OVERFLOW_BITMASK;    
};

extern SimpleTask Task;


//#define SIMPLETASK_DEBUG

SimpleTask::SimpleTask(){}

task_t* SimpleTask::create(void (*callback)(task_t &),int interval)
{
    if(taskCount<MAX_TASK){
        struct task_t tk = {callback,NULL,interval,true}; 
        tasks[taskCount++] = tk;
        return &tk;
    }
} 

task_t* SimpleTask::create(void (*callback)(void),int interval)
{
    if(taskCount<MAX_TASK){
        struct task_t tk = {NULL,callback,interval,true}; 
        tasks[taskCount++] = tk;
        return &tk;
    }
}
void SimpleTask::loop(){
    loop(millis());
}

void SimpleTask::loop(unsigned long t){    
    static bool first = true;
    for(int i=0;i<taskCount;i++){
        if(!tasks[i].enabled) continue;
        if(first){
            tasks[i].offset = t;
            tasks[i].next = t;
        }        
        t &= overflow_bitmask;
        if(t<=tasks[i].interval && tasks[i].next>overflow_bitmask){
            tasks[i].next &= overflow_bitmask;
        }
        #ifdef SIMPLETASK_DEBUG
        Serial.print("t="); 
        Serial.print(t);
        Serial.print("\tnext=");        
        Serial.println(tasks[i].next);
        #endif
        if(first || tasks[i].interval==0 || t>=tasks[i].next){
            unsigned long d = t - tasks[i].prev;            
            if(t<tasks[i].timestamp){
                d += overflow_bitmask+1; 
            }
            tasks[i].timestamp = t;                        
            if(tasks[i].offset==0)tasks[i].offset = t-tasks[i].interval;
                        
            tasks[i].timeoffset += d;
            if(first){
                tasks[i].offset = t;
                tasks[i].timeoffset -= tasks[i].offset; 
            }
            tasks[i].prev = t;
            tasks[i].next += tasks[i].interval;
            if(tasks[i].next<t) tasks[i].next = t+ tasks[i].interval;

            if(tasks[i].callback2!=NULL)
                tasks[i].callback2();
            else
                tasks[i].callback(tasks[i]); 
                
            #ifdef SIMPLETASK_DEBUG
            Serial.print("Event!! t=");
            Serial.println(tasks[i].timeoffset);
            #endif
        } 
    }   
    first = false;
}

SimpleTask Task;
#endif /*_SIMPLE_TASK_H*/
