#ifdef USE_LCD_1602
//I2C LCD Address 0x27 or 0x3F
const uint8_t i2c_lcd_scan_address[] = {0x27,0x3F};
#include "src/LiquidCrystal_I2C.h"
LiquidCrystal_I2C lcd;  
void task_update_clock();

void displayConfigMode(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("[AP Config Mode]");
  lcd.setCursor(0,1);
  lcd.print("SSID:");
  lcd.print(getEspName().c_str());
}

void setup_lcd(){
  bool found_lcd =false;
  for(int i=0;i<sizeof(i2c_lcd_scan_address);i++){
    uint8_t address = i2c_lcd_scan_address[i];
    Wire.beginTransmission(address);
    int error = Wire.endTransmission();
    if (error == 0){
      lcd.setAddress(address);
      found_lcd = true;
      break; 
    }
  }
  if(!found_lcd){
    Serial.println("Not found lcd connected!!");
    return;
  }
  lcd.begin(16,2);
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Initialize...");
  Task.create(task_update_clock,1000);  
}

void task_update_clock(){
  DateTime dt(now());    
  static char _buf[17];
  char buf[17];
  buf[16] = 0;
  printDateTime(now(),buf);
  int i = 0;
  for(i=0;i<16;i++){
    if(buf[i] != _buf[i])break;
  }
  if(i<16){
    lcd.setCursor(i,0);
    lcd.print(buf + i);
    memcpy(_buf,buf,16);
  } 
  static float temp = 0;
  if(temp!=temperature){  
    snprintf(buf,16,"T:%2.1f%cC",temperature,223);
    lcd.setCursor(0,1);
    lcd.print(buf);
    temp=temperature;
  }
  static float humi = 0;
  if(humi!=humidity){  
    snprintf(buf,16,"H:%2.1f%%",humidity);
    lcd.setCursor(9,1);
    lcd.print(buf);
    humi = humidity;
  }
}

#endif
