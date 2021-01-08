/******************************************/
/* Setup PCF8574 IO-Expander              */
/******************************************/ 
void setup_io_exp(){
  ex.setOutputChannels(4,exp_output_pin);
  ex.setInputChannels(4,exp_input_pin);
  ex.begin();
  ex.onInputChange(onInputChanged);
  
  sstm[0].onStart([]{onTimerStart(0);});
  sstm[0].onStop([]{onTimerStop(0);});
  sstm[1].onStart([]{onTimerStart(1);});
  sstm[1].onStop([]{onTimerStop(1);});
  sstm[2].onStart([]{onTimerStart(2);});
  sstm[2].onStop([]{onTimerStop(2);});
  sstm[3].onStart([]{onTimerStart(3);});
  sstm[3].onStop([]{onTimerStop(3);});

  Task.create([]{
    for(int i=0;i<4;i++){
      if(sstm[i].isEnabled()){
       sstm[i].loop();
      }
    }
  },500);

  Task.create([]{
    uint8_t _outbuf = ex.outputBuffer();
    static uint8_t outbuf = _outbuf;
    if(outbuf != _outbuf){
      for(int i=0;i<4;i++){
        if(!(outbuf & _outbuf & (1<<i)))
        {
          bool state = _outbuf & (1<<i);
          Blynk.virtualWrite(i,state?0:1);
          Blynk.virtualWrite(V20+i, state?0:255);
        }
      }
      outbuf = _outbuf;
    }
    ex.inputLoop();
  },100);
}

void onInputChanged(){
    for(int i=0;i<4;i++){
      struct InputStatus s = ex.getInputStatus(i);
      if(s.state == INPUT_STATE_RELEASED){           
        int n = ex.toggle(i);
        Serial.printf("Toggle Relay %u : %s\n",i+1,n?"OFF":"ON");
      }
    }
}

void onTimerStart(int n){
  Serial.printf("Relay %d Timer : ON\n",n+1);
  ex.write(n,0);
}

void onTimerStop(int n){
  Serial.printf("Relay %d Timer : OFF\n",n+1);
  ex.write(n,1);
}
