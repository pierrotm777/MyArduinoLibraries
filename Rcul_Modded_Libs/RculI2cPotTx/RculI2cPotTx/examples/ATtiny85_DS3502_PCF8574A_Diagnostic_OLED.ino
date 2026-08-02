#include <Arduino.h>
#include <TinyWireM.h>
#include <Tiny4kOLED.h>

/*
  ATtiny85 diagnostic:
    - DS3502 at 0x28
    - PCF8574A at 0x38
    - PROP analog input on A3
    - SSD1306 128x64 OLED
    - LED on PB1 / Arduino pin 1

  This sketch does NOT use RculI2cPotTx or RcTxSerial.
  It isolates the I2C peripherals and the analog input.

  DS3502:
    - wiper alternates between 5 and 122 every 3 seconds
    - register 0x00 and value are written in the same transaction

  PCF8574A:
    - 0xFF is written once to release all pins as inputs
    - one byte is read repeatedly
    - closed switch to GND becomes logical 1 after inversion

  OLED refresh is limited to 500 ms.
*/

#define LED_PIN             1
#define PROP_PIN            A3

#define DS3502_ADDRESS      0x28
#define PCF8574A_ADDRESS    0x38

#define DS_WIPER_LOW        5
#define DS_WIPER_HIGH       122

#define DS_TOGGLE_MS        3000UL
#define OLED_REFRESH_MS     500UL

static uint8_t CurrentWiper = DS_WIPER_LOW;
static uint8_t DsWriteRaw = 0xFF;
static uint8_t PcfWriteRaw = 0xFF;
static uint8_t PcfReadCount = 0;
static uint8_t PcfRaw = 0xFF;
static uint8_t Contacts = 0x00;

static uint16_t PropAdcRaw = 0;
static uint8_t Prop = 0;

static uint32_t LastDsToggleMs = 0;
static uint32_t LastOledMs = 0;

/*
  TinyWireM.endTransmission() raw value is displayed as-is.
  In your tests, 0x00 indicated a successful DS3502 transaction.
*/
static uint8_t ds3502WriteWiper(uint8_t Wiper)
{
  if(Wiper > 127U)
  {
    Wiper = 127U;
  }

  TinyWireM.beginTransmission(DS3502_ADDRESS);
  TinyWireM.send((uint8_t)0x00);
  TinyWireM.send(Wiper);

  return TinyWireM.endTransmission();
}

static uint8_t ds3502ConfigureVolatile(void)
{
  TinyWireM.beginTransmission(DS3502_ADDRESS);
  TinyWireM.send((uint8_t)0x02);
  TinyWireM.send((uint8_t)0x80);

  return TinyWireM.endTransmission();
}

static uint8_t pcf8574aReleaseInputs(void)
{
  TinyWireM.beginTransmission(PCF8574A_ADDRESS);
  TinyWireM.send((uint8_t)0xFF);

  return TinyWireM.endTransmission();
}

static bool pcf8574aRead(uint8_t &Value)
{
  const uint8_t Count =
      TinyWireM.requestFrom((uint8_t)PCF8574A_ADDRESS, (uint8_t)1);

  PcfReadCount = Count;

  if((Count != 1U) || !TinyWireM.available())
  {
    return false;
  }

  Value = TinyWireM.receive();
  return true;
}

static void printHexByte(uint8_t Value)
{
  if(Value < 0x10U)
  {
    oled.print('0');
  }

  oled.print(Value, HEX);
}

static void updateDisplay(void)
{
  oled.clear();
  oled.setFont(FONT6X8);

  oled.setCursor(0, 0);
  oled.print(F("ATTINY I2C DEBUG"));

  oled.setCursor(0, 1);
  oled.print(F("DS W="));
  oled.print(CurrentWiper);
  oled.print(F(" ret="));
  printHexByte(DsWriteRaw);

  oled.setCursor(0, 3);
  oled.print(F("PCF wr="));
  printHexByte(PcfWriteRaw);
  oled.print(F(" cnt="));
  oled.print(PcfReadCount);

  oled.setCursor(0, 4);
  oled.print(F("PCF raw=0x"));
  printHexByte(PcfRaw);

  oled.setCursor(0, 5);
  oled.print(F("SW =0x"));
  printHexByte(Contacts);

  oled.setCursor(0, 6);
  oled.print(F("ADC="));
  oled.print(PropAdcRaw);

  oled.setCursor(0, 7);
  oled.print(F("PROP="));
  oled.print(Prop);
}

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(PROP_PIN, INPUT);

  delay(500);

  TinyWireM.begin();

  oled.begin(128, 64,
             sizeof(tiny4koled_init_128x64br),
             tiny4koled_init_128x64br);

  oled.clear();
  oled.on();
  oled.setFont(FONT6X8);

  oled.setCursor(0, 0);
  oled.print(F("ATTINY I2C DEBUG"));
  oled.setCursor(0, 2);
  oled.print(F("Initialisation..."));

  DsWriteRaw = ds3502ConfigureVolatile();
  delay(20);

  DsWriteRaw = ds3502WriteWiper(CurrentWiper);
  delay(20);

  PcfWriteRaw = pcf8574aReleaseInputs();

  LastDsToggleMs = millis();
  LastOledMs = millis();

  delay(500);
}

void loop()
{
  const uint32_t NowMs = millis();

  /*
    Read PCF8574A continuously.
  */
  uint8_t NewPcfRaw = PcfRaw;

  if(pcf8574aRead(NewPcfRaw))
  {
    PcfRaw = NewPcfRaw;
    Contacts = (uint8_t)~PcfRaw;
  }

  /*
    Read PROP analog input.
  */
  PropAdcRaw = (uint16_t)analogRead(PROP_PIN);
  Prop = (uint8_t)(((uint32_t)PropAdcRaw * 255UL + 511UL) / 1023UL);

  /*
    Alternate DS3502 wiper every 3 seconds.
  */
  if((uint32_t)(NowMs - LastDsToggleMs) >= DS_TOGGLE_MS)
  {
    LastDsToggleMs = NowMs;

    CurrentWiper =
        (CurrentWiper == DS_WIPER_LOW) ? DS_WIPER_HIGH : DS_WIPER_LOW;

    DsWriteRaw = ds3502WriteWiper(CurrentWiper);

    if(DsWriteRaw == 0)
    {
      digitalWrite(LED_PIN, HIGH);
      delay(30);
      digitalWrite(LED_PIN, LOW);
    }
  }

  /*
    Refresh OLED slowly so it does not dominate the I2C bus.
  */
  if((uint32_t)(NowMs - LastOledMs) >= OLED_REFRESH_MS)
  {
    LastOledMs = NowMs;
    updateDisplay();
  }
}
