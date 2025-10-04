
/*
 * SimplePoll_7Sensors.ino - MPX_MSB v2.0.0 (simplified)
 * Tested target: Multiplex RX-9-DR + Teensy 4.0 (Serial3 @38400)
 * Wiring (Option B, recommended):
 *   Teensy TX1 (pin 14) -> 1k -> diode (anode MCU -> cathode RX) -> RX B/D (data)
 *   GND common. B/D is RX input.
 */
#include <MPX_MSB.h>
using namespace MPX;

Mpx_Msb mpx;

void setup(){
  mpx.begin(Serial3, 38400);

  // Values you want to expose (direct numbers)
  mpx.sendVbat(15.0f);  // addr 3 by default
  mpx.sendTmp1(25.0f);  // addr 6 by default
  mpx.sendTmp2(30.0f);  // addr 7 by default

  // Four digital alarms on Teensy pins 18,19,22,23 → addrs 9..12
  mpx.addAlarmDigital(18,  9, MPX_LIQUID);  // 0/1 with alarm bit when active
  mpx.addAlarmDigital(19, 10, MPX_LIQUID);
  mpx.addAlarmDigital(22, 11, MPX_LIQUID);
  mpx.addAlarmDigital(23, 12, MPX_LIQUID);

  // Timing tweaks
  mpx.setIdleMicros(300);
  mpx.setEchoMasking(true);
}

void loop(){
  // Refresh values here if needed (read ADCs, sensors, etc.) then:
  mpx.poll(); // answers only when the RX polls an address (poll byte & 0x0F)
}
