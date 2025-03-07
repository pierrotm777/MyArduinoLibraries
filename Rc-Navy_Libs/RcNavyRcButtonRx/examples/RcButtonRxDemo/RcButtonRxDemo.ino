#include <SoftRcPulseOut.h>
#include <SoftRcPulseIn.h>
#include <RcButtonRx.h>

static SoftRcPulseOut RcSignalTx;
static SoftRcPulseIn  RcSignalRx;
static RcButtonRx     Buttons;

/* 
   Custom Keyboard replacing a pot in the RC transmitter or using a free channel in the RC transmitter
   
   470   470   470   470   470   470   470   470   470
 .-###-o-###-o-###-o-###-o-###-o-###-o-###-o-###-o-###-.
 |     |     |     |     |     |     |     |     |     |
 | BP1(  BP2(  BP3(  BP4(  BP5(  BP6(  BP7(  BP8(      |
 |     |     |     |     |     |     |     |     |     |
 o-###-o-----o-----o-----o--o--o-----o-----o-----o-###-o
 | 100K                     |                     100K |
 '-----------------------.  |  .-----------------------'
                         |  |  |
                       - o  oC o +
                         |  |u |
                         |  |r |         .---------------.(Protection)
                         |  |s |         |               |    1K
                         |  |o '---------o +5V         2 >----###---. Pin 2 simulates the RC signal at RC Receiver side
                         |  |r           |               |          |  
                         |  '------------> A0            |          |
                         |               |               |          |
                         '---------------o GND         8 <----------' Pin 8 simulates a decoder input
                                         |               |
                                         '---------------'
                                            ARDUINO UNO
How this sketch works:
=====================
1) The voltage is read on A0 where the keyboard (with the buttons) is connected to.
2) A PWM RC pulse is generated on pin 2. The pulse width is scaled between 1000 us and 2000 us according to the voltage read on A0
3) The PWM RC signal is read by the <RcButtonRx> library on pin 8
4) To calibrate the system, a C command is expected in the serial console
5) Then, BP1: is displayed
6) Whilst maintaining pressed the BP1, hit Enter in the Serial console, this will measure the pulse width and will memorized in in EEPROM
7) Repeat the process for all the buttons. Hitting Enter for the last button will automatically exit from the calibration mode.
8) Sending a 'B' in the serial console will display the pulse width memorized in EEPROM for all the buttons
9) Then, pressing a button will set the associated outpout bit in ButtonOutputs variable.
10) Use each bit of the ButtonOutputs variable to set actions (eg: setting digital ouput pins)
*/

#define RC_SIGNAL_TX_PIN                2
#define RC_SIGNAL_RX_PIN                8

#define KEYBOARD_CURSOR_PIN             A0

#define RC_CHANNEL_ID                   0 /* 0 as it is a PWM RC Signal */

#define BUTTON_NB                       8 /* Put here the number of button of your keyboard */

#define RCUL_CLIENT_IDX                 5 /* If you don't know what it is, put 5 */

#define RC_BUTTON_EEPROM_BASE_ADDR      0 /* Address in EEPORM to store the calibration values */

#define TERMINAL_LINE_TERMINATOR_CR_LF  0
#define TERMINAL_LINE_TERMINATOR_CR     1

#define TERMINAL_LINE_TERMINATOR        TERMINAL_LINE_TERMINATOR_CR_LF

#define NORMAL_MODE                     0 // In Normal Mode, the command associated follows the status of the Button 
#define PULSE_MODE                      1 // In Pulse Mode,  the command associated toggles each time the Button is pressed

void setup()
{
  Serial.begin(115200);
  Serial.println(F("RcButtonRx Demo"));
  
  RcSignalTx.attach(RC_SIGNAL_TX_PIN);
  RcSignalRx.attach(RC_SIGNAL_RX_PIN);
  
  Buttons.begin(&Serial, TERMINAL_LINE_TERMINATOR, &RcSignalRx, RC_CHANNEL_ID, BUTTON_NB, RCUL_CLIENT_IDX, RC_BUTTON_EEPROM_BASE_ADDR);

  Buttons.setPulseMode(1, NORMAL_MODE); // -
  Buttons.setPulseMode(2, NORMAL_MODE); //  | Buttons 1, 2, 3 and 4 are set in Normal Mode
  Buttons.setPulseMode(3, NORMAL_MODE); //  |
  Buttons.setPulseMode(4, NORMAL_MODE); // -
  
  Buttons.setPulseMode(5, PULSE_MODE);  // -
  Buttons.setPulseMode(6, PULSE_MODE);  //  | Buttons 5, 6, 7 and 8 are set in Pulse Mode
  Buttons.setPulseMode(7, PULSE_MODE);  //  |
  Buttons.setPulseMode(8, PULSE_MODE);  // -

  // The following line defines the mode of all the actions associated to the Push-Buttons: it can replace the 8 previous lines in a single line of code
  Buttons.setPulseMap(B11110000); // Each bit at 1 puts the action of the button in Pulse mode. Each bit at 0 puts the action of the button in Normal mode
}

void loop()
{
  uint16_t        KeyboardCursor, PwmPulseWidthUs, ButtonOutputs;
  static uint16_t PrevButtonOutputs = 0xFFFF;

  /* Wait for a 'C' character sent in the Serial Terminal to switch in the Button calibration mode */
  /* Sending a 'B' will display the pulse width memorized in EEPROM for all the buttons */
  if(!Buttons.isInCalibrationMode())
  {
    if(Serial.available())
    {
      switch (Serial.read())
      {
        /* Minimal command interpretor */
        case 'C':
        Buttons.enterInCalibrationMode();
        break;
        
        case 'B':
        Buttons.displayButtonPulseWidth();
        break;
      }
    }
  }
  
  /* Read the cursor pin of the Keyboard with buttons and generate a PWM RC signal */
  if(SoftRcPulseOut::refresh()) /* SoftRcPulseOut::refresh() returns 1 every 20 ms */
  {
    /* This section of code simulates the RC transmitter and the RC receiver */
    KeyboardCursor  = analogRead(KEYBOARD_CURSOR_PIN);
    PwmPulseWidthUs = map(KeyboardCursor, 0, 1023, 1000, 2000);
    RcSignalTx.write_us(PwmPulseWidthUs); /* Produces a PWM signal according to the analog value on the cursor of the keyboard */
  }

  /* Button Management: it reads the PWM RC signal on the RC_SIGNAL_RX_PIN pin  */
  ButtonOutputs = Buttons.process(); /* Each bit of ButtonOutputs represents the status of the command associated to each button */
  if(!Buttons.isInCalibrationMode())
  {
    if(ButtonOutputs != PrevButtonOutputs)
    {
      PrevButtonOutputs = ButtonOutputs;
      displayButtonValues(ButtonOutputs);
      /* Use here each bit of the ButtonOutputs variable: bit0 is associated to BP1, bit1 is associated to BP2, etc... */
    }
  }

}


void displayButtonValues(uint16_t ButtonOutputs)
{
  if(BUTTON_NB >= 10)
  {
    Serial.print(F("             "));
    uint8_t TwoDigitNb = (BUTTON_NB + 1) - 10;
    for(uint8_t i = 0; i< TwoDigitNb; i++) Serial.print(F("1"));
    Serial.println();
  }
  Serial.print(F("But:"));
  for(uint8_t ButtonIdx = 0; ButtonIdx < BUTTON_NB; ButtonIdx++)
  {
    Serial.print((ButtonIdx + 1) < 10? (ButtonIdx + 1): (ButtonIdx + 1) - 10);
  }
  Serial.println();
  
  Serial.print(F("Val:"));
  for(uint8_t ButtonIdx = 0; ButtonIdx < BUTTON_NB; ButtonIdx++)
  {
    Serial.print(bitRead(ButtonOutputs, ButtonIdx));
  }
  Serial.println();
}

