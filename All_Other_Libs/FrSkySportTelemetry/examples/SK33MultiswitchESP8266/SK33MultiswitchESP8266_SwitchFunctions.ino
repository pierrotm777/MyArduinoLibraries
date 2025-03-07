void Fahrt() {
  if ((uiValue[0] & 0x01) != 0) {
    display.setCursor(0, 16);
    display.print("F");       
    
    module1onOff[0] = true; //heck
    module1onOff[2] = true; //navSB
    module2onOff[0] = true; //topp
    module2onOff[2] = true; //navBB
  }
}

void Schleppfahrt() {
  if ((uiValue[0] & 0x02) != 0) {
     
    display.setCursor(0, 16);
    display.print("S");       
    
    module1onOff[0] = true; //heck
    module1onOff[1] = true; //heckSchlepp
    module1onOff[2] = true; //navSB
    module2onOff[0] = true; //topp
    module2onOff[1] = true; //toppschlepp
    module2onOff[2] = true; //navBB
  }
}

void Blaulicht() {
  if ((uiValue[0] & 0x04) != 0) {
    display.setCursor(96, 16);
    display.print("B");
    bluelightActive = true;
  } else {
    bluelightActive = false;
    pwmLed1.setPin(13, 0, false);
    pwmLed2.setPin(13, 0, false);
    pwmLed1.setPin(14, 0, false);
    pwmLed2.setPin(14, 0, false);
    pwmLed1.setPin(15, 0, false);
    pwmLed2.setPin(15, 0, false);
  }
}

void Manoevrierbehindert() {
  if ((uiValue[1] & 0x01) != 0) {
    display.setCursor(16, 16);
    display.print("M");       
    
    module1onOff[3] = true; //SBsignalO
    module1onOff[4] = true; //SBsignalM
    module1onOff[5] = true; //SBsignalU
    module2onOff[3] = true; //BBsignalO
    module2onOff[4] = true; //BBsignalM
    module2onOff[5] = true; //BBsignalU
  }
}
