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

#ifndef _SERIALCOMMAND_H
#define _SERIALCOMMAND_H
#include "HardwareSerial.h"

class SerialCommand{
  public:
    SerialCommand();
    SerialCommand(void*serial);
    void registerCommand(char*cmd,void (*callback)(const char * arg));
    void read(void);
  private:
    void* _serial;
    int cmdCount = 0;
    int maxCmd = 32;
    struct SerialCommandCallback{
      const char * cmd;
      void (*callback)(const char * arg);
    };
    SerialCommandCallback * commandList;
    char arg_buf[1000];
    char cmd_buf[32];
    char*arg_ptr;
    char*cmd_ptr;
    bool has_cmd = false;    
};


#endif
