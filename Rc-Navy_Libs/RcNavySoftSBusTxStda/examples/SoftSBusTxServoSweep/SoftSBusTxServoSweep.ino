#include <SoftSBusTxStda.h>

// IMPORTANT: For timing reasons, you *have to* defined the SBUS output pin in SoftSBusTxStda.h of the library using the Port name and Bit number
// For example, in SoftSBusTxStda.h, if SOFT_SBUS_TX_PIN_PORT_LETTER is set to B and SOFT_SBUS_TX_PIN_BIT_IN_PORT is set to 1 which means pin B1,
// it coressponds to pin 9 of the Arduino UNO

// 1) Define below the SBUS frame period in ms: either SOFT_SBUS_TX_NORMAL_FRAME_RATE_MS or SOFT_SBUS_TX_HIGH_SPEED_FRAME_RATE_MS
#define SOFT_SBUS_TX_PERIOD_MS  SOFT_SBUS_TX_NORMAL_FRAME_RATE_MS

// 2) Define below the SBUS polarity: either SOFT_SBUS_TX_POLAR_FUTABA or SOFT_SBUS_TX_POLAR_FUTABA_INV (avoiding hardware inverter)
#define SOFT_SBUS_TX_POLAR      SOFT_SBUS_TX_POLAR_FUTABA


#define STEP_US                 8 // Define the speed of the sweep (higher the step is, lower the sweep is)

#define UP_DIRECTION            (+1 * STEP_US)
#define DOWN_DIRECTION          (-1 * STEP_US)

#define WIDHT_US_MIN            1000
#define WIDHT_US_MAX            2000

int width_us  = WIDHT_US_MIN;    // variable to store the servo position 
int step      = UP_DIRECTION;

void setup()
{
  Serial.begin(115200);
  Serial.println(F("\nSoftSBusTxServoSweep demo"));
  SoftSBusTx.begin(SOFT_SBUS_TX_PERIOD_MS, SOFT_SBUS_TX_POLAR);
}

void loop()
{
  if(SoftSBusTx.process()) // SoftSBusTx.process() will send the SBUS frame every SOFT_SBUS_TX_PERIOD_MS milliseconds
  {
    // We arrive here at every end of SBUS frame: if you absolutely need to update channel(s) at every SBUS frame,
    // you have X ms to set the different SBUS channels (X=4 ms in SBUS high speed or X=10 ms in SBUS normal speed)
    width_us += step;
    if(width_us >= WIDHT_US_MAX) step = DOWN_DIRECTION; // As soon as 2000 us are reached -> Change direction
    if(width_us <= WIDHT_US_MIN) step = UP_DIRECTION;   // As soon as 1000 us are reached -> Change direction
    for(uint8_t ChannelId = 1; ChannelId <= SOFT_SBUS_TX_CHANNEL_NB; ChannelId++)
    {
      SoftSBusTx.width_us(ChannelId, width_us); // Apply the witdh to the all the 16 SBUS channels (to be sure the SERVO will be feed with the width)
    }
    Serial.print(F("width="));Serial.println(width_us);
  }
  // Otherwise you can set channel(s) here if you have time
}
