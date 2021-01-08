#ifdef USE_DHT22
#define DHT22_PIN   D0
#include <SimpleDHT.h>
SimpleDHT22 dht(DHT22_PIN);
void setup_dht(){
  Task.create([]{
    int err = SimpleDHTErrSuccess;
    if ((err = dht.read2(&temperature, &humidity, NULL)) == SimpleDHTErrSuccess) {
      if(humidity>=100)humidity=99.9f;
      Serial.printf("Temp:%.2f,Humidity:%.2f\n",temperature,humidity);
    }else{      
    }
  },1000);
}
#endif
