#include <H_BridgeCmd.h>

#define CMD1_PIN  5 // PD5 is Arduino pin 5
#define CMD2_PIN  6 // PD6 is Arduino pin 6

enum {DIR_FORWARD = 0, DIR_REAR};

static H_BridgeCmd H_Bridge(CMD1_PIN, CMD2_PIN);

void setup()
{
    Serial.begin(115200);
    Serial.print(F("H_BridgeCmd lib V"));Serial.print(H_BRIDGE_CMD_VERSION);Serial.print(F("."));Serial.print(H_BRIDGE_CMD_REVISION);Serial.println(F(" demo"));
}

void loop()
{
  /* Forward direction */
  H_Bridge.command(DIR_FORWARD, 64);  /* 25% of max speed */
  delay(2000);
  H_Bridge.command(DIR_FORWARD, 128); /* 50% of max speed */
  delay(4000);
  H_Bridge.command(DIR_FORWARD, 64);  /* 25% of max speed */
  delay(2000);
  H_Bridge.command(DIR_FORWARD, 0);   /* 0%  of max speed */
  delay(2000);

  /* Rear direction */
  H_Bridge.command(DIR_REAR, 64);     /* 25% of max speed */
  delay(2000);
  H_Bridge.command(DIR_REAR, 128);    /* 50% of max speed */
  delay(4000);
  H_Bridge.command(DIR_REAR, 64);     /* 25% of max speed */
  delay(2000);
  H_Bridge.command(DIR_REAR, 0);      /* 0%  of max speed */
  delay(2000);
}


