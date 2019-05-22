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
// Library Name : EasyEEPROM for Arduino
// Description : Read/Write byte or struct to I2C EEPROM (base on AT24CXXX)
// Architecture : Support All (AVR/ESP8266/STM32)
// Version : 0.1
// Owner : Kamon Singtong (MakeArduino.com)
// email : kamon.dev@hotmail.com
// fb : makearduino
//***********************************************************************************

#include "eeprom.h"
#include <Arduino.h>

//#define DEBUG
#include "debug.h"

EasyEEPROM::EasyEEPROM(TwoWire *wire,int deviceAddress)
    :_wire(wire),_deviceAddress(deviceAddress)
{

}

EasyEEPROM::EasyEEPROM(int deviceAddress)
    :EasyEEPROM(&Wire,deviceAddress)
{

}
void EasyEEPROM::begin(){
    _wire->beginTransmission(_deviceAddress);
    _isValid = (_wire->endTransmission()==0);
}

bool EasyEEPROM::isValid(){
    return _isValid;
}

unsigned char EasyEEPROM::read(unsigned int address){
    int rdata = 0xFF;
    _wire->beginTransmission(_deviceAddress);
    _wire->write((int)(address >> 8)); // MSB
    _wire->write((int)(address & 0xFF)); // LSB
    _wire->endTransmission();
    _wire->requestFrom(_deviceAddress,1);
    if (_wire->available()) rdata = _wire->read();
    delay(EEPROM_RW_DELAY);
#ifdef DEBUG
    SerialDebug_printf("Read EEPROM 1 byte to address %04X : %02X\n",address,(unsigned char)(rdata&0xFF));
#endif
    return (unsigned char)(rdata&0xFF);
}
 
void EasyEEPROM::write(unsigned int address,unsigned char data){
#ifdef DEBUG
    SerialDebug_printf("Write EEPROM 1 byte to address %04X : %02X\n",address,data);
#endif
    int rdata = data;
    _wire->beginTransmission(_deviceAddress);
    _wire->write((int)(address >> 8)); // MSB
    _wire->write((int)(address & 0xFF)); // LSB
    _wire->write(rdata);
    _wire->endTransmission(); 
    delay(EEPROM_RW_DELAY);    
}

void EasyEEPROM::read(unsigned int address,void *buffer,int length){ 
    unsigned char * _buf = (unsigned char *) buffer;
    int remain = length;
#ifdef DEBUG
    int h = 0;
    SerialDebug_printf("Read EEPROM %d bytes from address %04X:\n",length,address);
#endif
    while(remain>0){
        int n = EEPROM_BLOCK_SIZE;
        if(n>remain){
            n=remain;
            remain =0;
        }else{
            remain -=n;
        }        
        _wire->beginTransmission(_deviceAddress);
        _wire->write((int)(address >> 8)); // MSB
        _wire->write((int)(address & 0xFF)); // LSB
        _wire->endTransmission();
        _wire->requestFrom(_deviceAddress,n);
        address+=n;
        for (int c = 0; c < n; c++ ){
            if (_wire->available()) *_buf++ = _wire->read()&0xff;                    
#ifdef DEBUG            
            SerialDebug_printf("%02X ",((unsigned char *)buffer)[h]);
            if((++h % 16)==0){
                SerialDebug_print("\n");
            }
#endif                        
        }
        delay(EEPROM_RW_DELAY);        
    }
#ifdef DEBUG
    SerialDebug_print("\n");
#endif
}

void EasyEEPROM::write(unsigned int address,void *buffer,int length){
#if EEPROM_WRITE_PAGE > 0    
    unsigned char * _buf = (unsigned char *)buffer;
    int remain = length;
    int s = address;
    int h = 0;
#ifdef DEBUG
    SerialDebug_printf("Write EEPROM %d bytes to address %04X:\n",length,address);
    long t_ms=0;
    s = (address/EEPROM_PAGE_SIZE)*EEPROM_PAGE_SIZE;
    while(s<address){
        SerialDebug_print("-- ");
        if(++s%16 == 0)SerialDebug_print("\n");
    }
#endif    
    while(h<length){
#ifdef DEBUG   
        long _ms = millis();     
#endif
        int b = 0;
        _wire->beginTransmission(_deviceAddress);
        _wire->write((int)(s >> 8)); // MSB
        _wire->write((int)(s & 0xFF)); // LSB
        for (;;){
            Wire.write(*_buf++);
            s++;
#ifdef DEBUG            
            SerialDebug_printf("%02X ",((unsigned char *)buffer)[h]);
            if((s % 16)==0){
                SerialDebug_print("\n");
            }
#endif      
            h++;
            b++;
            if(s%EEPROM_PAGE_SIZE==0)
            {
#ifdef DEBUG                   
                for(int i=0;i<16;i++)SerialDebug_print("---");
                SerialDebug_print("\n");
#endif                      
                break;
            }
            
            if(h>=length)break;
            if(b>=EEPROM_BLOCK_SIZE)break;
        }                
        _wire->endTransmission();
        delay(EEPROM_RW_DELAY);
#ifdef DEBUG            
        t_ms += millis() - _ms;
#endif                      
    }
#ifdef DEBUG
    SerialDebug_printf("\nFinished (%d ms.)\n",t_ms);
#endif
    
#else
    #ifdef DEBUG        
        SerialDebug_printf("Write EEPROM %d bytes to address %04X:\n",length,address);
        long t_ms=0;
    #endif
    for(int i=0;i<length;i++){
    #ifdef DEBUG   
        long _ms = millis();     
    #endif
        _wire->beginTransmission(_deviceAddress);
        _wire->write((int)((address+i) >> 8)); // MSB
        _wire->write((int)((address+i) & 0xFF)); // LSB
        _wire->write(((unsigned char *)buffer)[i]);
        _wire->endTransmission();                 
        delay(EEPROM_RW_DELAY);
    #ifdef DEBUG   
        t_ms += millis() - _ms;
        SerialDebug_printf("%02X ",((unsigned char *)buffer)[i]);
        if((i % 16)==15){
            SerialDebug_print("\n");
        }
#endif      
    }
    #ifdef DEBUG
        SerialDebug_printf("\nFinished (%d ms.)\n",t_ms);
    #endif
#endif


}
