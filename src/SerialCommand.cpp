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

#include "SerialCommand.h";
#include <Arduino.h>

SerialCommand::SerialCommand()
  :SerialCommand(&Serial)
{
}

SerialCommand::SerialCommand(void* serial)
:  _serial(serial)
{
    commandList = NULL;
    cmdCount = 0;
    cmd_ptr = cmd_buf;
    arg_ptr = arg_buf;

}

void SerialCommand::registerCommand(char*cmd,void (*callback)(const char * arg))
{
    int c = cmdCount++;
    commandList =  (SerialCommandCallback *) realloc(commandList, (c + 1) * sizeof(SerialCommandCallback));
    commandList[c].cmd = (const char*)cmd;
    commandList[c].callback = callback;
}

void SerialCommand::read(void){  
  while(((Stream*)_serial)->available()){
    char c = ((Stream*)_serial)->read(); 
    if(!has_cmd){
      if(cmd_ptr>cmd_buf && (c==' ' || c=='\n' ||c==':')){
        has_cmd = true; 
      }else{
        *cmd_ptr++ = c;
        *cmd_ptr = 0;
      } 
    }else if(c>=32){
      *arg_ptr++ = c;
      *arg_ptr = 0;
    }
    if(c=='\n'){
        for(int i = 0;i<cmdCount;i++){
          if(strcmp(commandList[i].cmd,cmd_buf)==0){
            commandList[i].callback(String(arg_buf).c_str());            
            break;
          }
        }        
        has_cmd = false;      
        arg_ptr = arg_buf;
        cmd_ptr = cmd_buf;
    }
  }
}  
