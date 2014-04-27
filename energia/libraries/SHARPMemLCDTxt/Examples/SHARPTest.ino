#include <SPI.h>
#include <SHARPMemLCDTxt.h>

SHARPMemLCDTxt display;       // configured for 43oh BoosterPack
//SHARPMemLCDTxt display(6,5);  // configured for official TI BoosterPack

char text_buffer[8];

void setup()
{
//  Turn on LCD power with offical TI BoosterPack
//  pinMode(2, OUTPUT);
//  digitalWrite(2, HIGH);

  display.begin(); // configure display
  
  display.clear();
  display.on();

  // write static text  
  display.print("HELLO WORLD!", 8);
  display.print("            ", 64, DISP_INVERT);
  display.print(" SHARP ", 72, DISP_INVERT + DISP_WIDE);
  display.print(" MEMORY LCD ", 80, DISP_INVERT);
  display.print("            ", 88, DISP_INVERT);
}

void loop()
{
  // wait a while
  delay(500);
  
  // update pseudo clock
  long time = millis() / 1000;
  
  int time_minute = time / 60 % 99;
  int time_second = time % 60;
 
  text_buffer[0] = ' ';
  text_buffer[1] = time_minute / 10 + '0';
  text_buffer[2] = time_minute % 10 + '0';
  text_buffer[3] = ':';
  text_buffer[4] = time_second / 10 + '0';
  text_buffer[5] = time_second % 10 + '0';
  text_buffer[6] = 0;

  display.print(text_buffer, 32, DISP_HIGH + DISP_WIDE);
}
