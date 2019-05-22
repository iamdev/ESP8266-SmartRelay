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

#ifndef _PCF8574_IOEXP_H
#define _PCF8574_IOEXP_H
#include <Wire.h>
#include <Arduino.h>

#ifdef DEBUG
#undef DEBUG
#endif
#include "debug.h"

enum InputState{
    INPUT_STATE_NONE,
    INPUT_STATE_DOWN,
    INPUT_STATE_RELEASED,
};

struct InputStatus{
    InputState state;
    long startTime;
    long duration;
};

class PCF8574{
    public: 
        PCF8574(TwoWire *wire,int deviceAddress);
        PCF8574(int deviceAddress);         
        void begin();
        int setInputChannels(int n,const int * bits);
        int setOutputChannels(int n,const int * bits);
        void onInputChange(void (*callback)(void));
        void inputLoop();
        int read(int ch);
        uint16_t read();
        void write(int ch,int state);  
        void write(uint16_t data);   
        int toggle(int ch);     
        int getOutputState(int ch);
        InputStatus getInputStatus(int ch);
        uint8_t outputBuffer() const {return _out_buffer;}
        uint8_t inputBuffer() const {return _in_buffer;}
    protected:
        uint16_t _read();
        void _write(uint16_t data);
        int _interruptPin=-1;
        int _deviceAddress;
        int _isValid = 0;
        int _inputCount=0,_outputCount=0;
        int _inputChannels[8];        
        int _outputChannels[8];
        int _input_bitmask;
        int _output_bitmask;
        int _out_buffer,_in_buffer;
        struct InputStatus _inputStatus[8];
        void (*_inputChangeCallback)(void);        
        TwoWire *_wire;
};

#endif

