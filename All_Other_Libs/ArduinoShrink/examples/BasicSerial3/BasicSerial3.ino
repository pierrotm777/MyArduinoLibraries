#include <BasicSerial3.h>

// sketch to test Serial

void setup()
{
}

void serOut(const char* str)
{
   while (*str) TxByte (*str++);
}

void loop(){
  byte c;
  serOut("Serial echo test\n\r");
  while ( c = RxByte() ){
    TxByte(c);
  }
  delay(1000);
}