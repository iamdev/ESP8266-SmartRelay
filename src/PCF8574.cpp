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

#include "PCF8574.h"
#include <Arduino.h>
#define PCF8574_I2C_ALTADDR 0x18

#ifndef print_binary
#ifdef DEBUG
void print_binary(int num,int nbit){
    int m = 1<<(nbit-1);
    SerialDebug_printf("%04X [",num);
    for(int i = 0;i<nbit;i++){        
        SerialDebug_print((num & m)?"1":"0");
        if((i+1)%4==0)SerialDebug_print(" ");
        m>>=1;
    }
    SerialDebug_printf("]");
}
#else
#define print_binary(num,nbit)
#endif
#endif

PCF8574::PCF8574(TwoWire *wire,int deviceAddress)
    :_wire(wire),_deviceAddress(deviceAddress)
{
    _wire->beginTransmission(_deviceAddress);
    _isValid = (_wire->endTransmission()==0);
}

PCF8574::PCF8574(int deviceAddress)
    :PCF8574(&Wire,deviceAddress)
{

}

void PCF8574::begin(){
    int addr = _deviceAddress;
    _wire->beginTransmission(addr);
    int error = _wire->endTransmission();
    SerialDebug_printf("Detect PCF8574 ...");
    if(error!=0){
        addr = _deviceAddress|PCF8574_I2C_ALTADDR;
        _wire->beginTransmission(addr);
        error = _wire->endTransmission();   
    }

    if(error==0){
        _deviceAddress = addr;
        SerialDebug_printf("Address %2X\n",addr);
        this->_isValid = 1;
    }else{
      SerialDebug_print("NOT FOUND!!\n"); 
    }

    if(_isValid){
        int data =_read(); 
        _in_buffer = (data|_output_bitmask)&0xFFFF;    
        _out_buffer = (data|_input_bitmask)&0xFFFF;    
    }
}

int PCF8574::setInputChannels(int n,const int * bits){
    _inputCount = n;
    _input_bitmask = 0;

    for(int i=0;i<n;i++){
        _inputChannels[i] = bits[i];
        _input_bitmask |= 1<<bits[i];
    }    
}

int PCF8574::setOutputChannels(int n,const int * bits){
    _outputCount = n;
    _output_bitmask = 0;
    for(int i=0;i<n;i++){
        _outputChannels[i] = bits[i];
        _output_bitmask |= 1<<bits[i];
    }    
}

uint16_t PCF8574::_read(){
    _wire->beginTransmission( _deviceAddress );
    _wire->endTransmission();
    _wire->requestFrom(_deviceAddress, (int)1 ); // request only one byte
    unsigned long t=millis();
    int data = 0xFFFF;
    while ( millis() < t+1000 && _wire->available() == 0 ); // waiting 
    if(_wire->available()){
        data = _wire->read();
    }
    _wire->endTransmission();  
    return data;
}

void PCF8574::_write(uint16_t data){
    _wire->beginTransmission(_deviceAddress);
    _wire->write(data&0xFF);
    _wire->endTransmission(); 
}

int PCF8574::read(int ch){
    if(ch<0 && ch>= _inputCount) return 0;
    int pin = _inputChannels[ch];
    SerialDebug_print("IO_EXP :: read data ");
    SerialDebug_print(_read(),BIN);
    SerialDebug_print("\n");
    return (_read()>>pin)&0x1;
}

uint16_t PCF8574::read(){
    int data = this->_read();
    SerialDebug_print("IO_EXP :: Read => ");
    print_binary(data,8);
    SerialDebug_print("\n");
    data &= _input_bitmask;
    int out = 0;
    for(int i=0;i<_inputCount;i++){
        int b = _inputChannels[i];
        out <<=1;        
        out |= ((data>>b)&1)?1:0;
    }
    SerialDebug_print("IO_EXP :: Out Channels => ");
    SerialDebug_print(out,BIN);
    SerialDebug_print("\n");
    return out;
}

void PCF8574::write(int ch,int state){
    SerialDebug_printf("EX -  Write!, ch %d : %d",ch,state);
    if(ch<0 && ch>= _outputCount)return;
    int pin = _outputChannels[ch];
    SerialDebug_printf("IO_EXP :: Write bit %d to %d\n",pin,state);
    SerialDebug_print("-----------------------------------\n");

    SerialDebug_print("  IO_EXP :: Output buffer =");
    SerialDebug_print(_out_buffer&0xFF,BIN);
    SerialDebug_print("\n");

    _out_buffer &= (~(1<<pin))&0xFFFF;
    _out_buffer |= (state?1:0)<<pin;

    int data = _out_buffer|_input_bitmask;
    SerialDebug_print("  IO_EXP :: Write data =");
    SerialDebug_print(data,BIN);
    SerialDebug_print("\n");
    SerialDebug_print("-----------------------------------\n");    
    _write(data);
}

void PCF8574::write(uint16_t data){    
    for(int i=0;i<_outputCount;i++){
        int pin = _outputChannels[i];
        _out_buffer &= (~(1<<pin))&0xFFFF;
        _out_buffer |= ((data&(1<<i))?1:0)<<pin;
    }
    this->_write(_out_buffer|_input_bitmask);
}

int PCF8574::toggle(int ch){
    if(ch<0 && ch>= _outputCount)return 0;
    int state = !getOutputState(ch);
    this->write(ch,state);
    return state;
}

int PCF8574::getOutputState(int ch){
    if(ch<0 && ch>= _outputCount)return 0;
    int state = _out_buffer>>_outputChannels[ch];
    return state&1;
}

InputStatus PCF8574::getInputStatus(int ch){
    if(ch>=0 && ch<_outputCount){
        return _inputStatus[ch];
    }
}


void PCF8574::onInputChange(void (*callback)(void)){
    _inputChangeCallback = callback;
}
void PCF8574::inputLoop(){
    if(!_isValid)return;
    int in = _read()|_output_bitmask;    
    in &=0xFFFF;
    if(in!= _in_buffer){
        for(int i=0;i<_inputCount;i++){
            int p = _inputChannels[i];
            int p1 = (_in_buffer>>p)&1;
            int p2 = (in>>p)&1;
            if(p1 && !p2){
                _inputStatus[i].startTime = millis();
                _inputStatus[i].state = INPUT_STATE_DOWN;
                _inputStatus[i].duration = 0;
            }else if (!p1 && p2){
                _inputStatus[i].state = INPUT_STATE_RELEASED;
                _inputStatus[i].duration = millis() - _inputStatus[i].startTime;
            }else if(p1 && p2){
                _inputStatus[i].state = INPUT_STATE_NONE;
            }
        }
        _in_buffer = in;
        if(_inputChangeCallback)_inputChangeCallback();
    }
}