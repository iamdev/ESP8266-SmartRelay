#ifndef _DEBUG_H
#define _DEBUG_H

#ifndef DEBUG 
//#define DEBUG
#endif


#ifdef DEBUG
    #include <HardwareSerial.h>
    #ifndef SerialDebug
        #define SerialDebug Serial
    #endif 
    #define SerialDebug_print(...) SerialDebug.print(__VA_ARGS__)
    #define SerialDebug_println(...) SerialDebug.println(__VA_ARGS__)
    #define SerialDebug_printf(...) { \
        char _buf[256]; \
        snprintf(_buf,256,__VA_ARGS__); \
        SerialDebug.print(_buf);\
    }
#else    
    #define SerialDebug_print(...)
    #define SerialDebug_println(...)
    #define SerialDebug_printf(...)
#endif
#endif /*_DEBUG_H*/