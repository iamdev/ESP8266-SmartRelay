#ifdef USE_LCD_1602
#define LCD_1602_ADDRESS 0x3F
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(LCD_1602_ADDRESS,16,2);  
void task_update_clock();

void setup_lcd(){
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
    Serial.print(buf);
    Serial.print(" ");
    Serial.print(buf + i);
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
