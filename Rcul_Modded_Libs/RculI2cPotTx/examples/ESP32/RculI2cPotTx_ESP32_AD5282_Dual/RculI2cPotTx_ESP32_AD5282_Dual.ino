#include <Arduino.h>
#include <Wire.h>
#include <ESP32_PPM.h>
#include <RcTxSerial.h>

#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>

#define SDA_PIN 5
#define SCL_PIN 6
#define PPM_INPUT_PIN 4
#define PROP1_PIN 0
#define ANGLE_PIN 1
#define PROP2_PIN 2
#define AD5282_ADDRESS 0x2C
#define MCP23017_ADDRESS 0x20
#define PPM_CHANNEL_NB 8
#define PPM_SYNC_MIN_US 3500

static RculI2cPotTxClass Pot0(
    AD5282, RCUL_I2C_POT_SYNCRO_BY_PPMIN, Cppm32,
    RCUL_I2C_POT_TX_AD5282_CHANNEL_0);

static RculI2cPotTxClass Pot1(
    AD5282, RCUL_I2C_POT_SYNCRO_BY_PPMIN, Cppm32,
    RCUL_I2C_POT_TX_AD5282_CHANNEL_1);

static RcTxSerial Tx0(&Pot0, 5, 8, 8); // SW16+PROP
static RcTxSerial Tx1(&Pot1, 5, 8, 9); // ANGLE+PROP

static uint16_t Contacts = 0;
static uint8_t Prop1 = 0, Prop2 = 0;
static uint16_t Angle = 0;

static bool writeMcp(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(reg); Wire.write(value);
  return Wire.endTransmission() == 0;
}

static bool beginMcp(void)
{
  return writeMcp(0x00, 0xFF) && writeMcp(0x01, 0xFF) &&
         writeMcp(0x0C, 0xFF) && writeMcp(0x0D, 0xFF);
}

static bool readMcp(uint16_t &v)
{
  Wire.beginTransmission(MCP23017_ADDRESS); Wire.write(0x12);
  if(Wire.endTransmission(false) != 0) return false;
  if(Wire.requestFrom((uint8_t)MCP23017_ADDRESS, (uint8_t)2) != 2) return false;
  v = (uint16_t)~((uint16_t)Wire.read() | ((uint16_t)Wire.read() << 8));
  return true;
}

void setup()
{
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN, 100000UL);
  analogReadResolution(12);

  Cppm32.beginRx(
      PPM_INPUT_PIN, PPM_CHANNEL_NB,
      true,             // impulsions PPM positives
      PPM_SYNC_MIN_US,  // 3500 us
      800, 2200);

  Pot0.begin(&Wire, AD5282_ADDRESS, 18, 1500);
  Pot1.begin(&Wire, AD5282_ADDRESS, 18, 1500);

  // Deux clients distincts sur la meme source Cppm32.
  Pot0.setPpmInSyncClientIdx(5);
  Pot1.setPpmInSyncClientIdx(6);

  // Deux tables Preferences independantes si calibration utilisee.
  Pot0.setTableStoragePreferences("RculAD0");
  Pot1.setTableStoragePreferences("RculAD1");

  beginMcp();
}

void loop()
{
  readMcp(Contacts);
  Prop1 = (uint8_t)((uint32_t)analogRead(PROP1_PIN) * 255UL / 4095UL);
  Angle = (uint16_t)analogRead(ANGLE_PIN);
  Prop2 = (uint8_t)((uint32_t)analogRead(PROP2_PIN) * 255UL / 4095UL);

  Pot0.process(); Pot1.process();

  if(Tx0.isReadyForTx())
  {
    uint8_t msg[3] = {Prop1, highByte(Contacts), lowByte(Contacts)};
    Tx0.sendNibbleMsg(msg, 6, 1);
  }

  if(Tx1.isReadyForTx())
  {
    uint8_t msg[3];
    msg[0] = (Angle & 0x0FF0) >> 4;
    msg[1] = ((Angle & 0x000F) << 4) | ((Prop2 & 0xF0) >> 4);
    msg[2] = (Prop2 & 0x0F) << 4;
    Tx1.sendNibbleMsg(msg, 5, 1);
  }

  RcTxSerial::process();
}
