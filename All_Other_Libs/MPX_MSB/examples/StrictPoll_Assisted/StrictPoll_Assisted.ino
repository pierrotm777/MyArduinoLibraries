/**
 * StrictPoll_Assisted.ino - MPX_MSB v1.9.7
 * Hardware: Multiplex RX-9-DR (M-LINK) + Teensy 4.0 (Serial3 38400)
 * Wiring option B (recommended): Teensy TX -> 1k -> diode (anode MCU -> cathode RX) -> RX B/D
 * GND common. B/D is an INPUT on RX.
 */
#include <MPX_MSB.h>
using namespace MPX;

Mpx_Msb mpx;

void setup(){
  mpx.begin(Serial3, 38400);

  // Declare only what you want to expose
  mpx.sendVbat(15.0f);  // addr 3, VOLT (0.1V scale in frame)
  mpx.sendTmp1(25.0f);  // addr 6, TMP  (0.1°C)
  mpx.sendTmp2(30.0f);  // addr 7, TMP

  // Four alarms (pullup, active LOW → value 1 when LOW)
  mpx.addAlarmDigital(18,  9, MPX_LIQUID);
  mpx.addAlarmDigital(19, 10, MPX_LIQUID);
  mpx.addAlarmDigital(22, 11, MPX_LIQUID);
  mpx.addAlarmDigital(23, 12, MPX_LIQUID);

  // Ensure we never reply outside these addresses
  mpx.setAllowMask( (1u<<3) | (1u<<6) | (1u<<7) | (1u<<9) | (1u<<10) | (1u<<11) | (1u<<12) );

  // Discovery assist is ON by default; can be turned off:
  // mpx.setDiscoveryAssist(false);
}

void loop(){
  // Update values here if needed (e.g., read ADC, sensors...), then:
  mpx.poll();
}
