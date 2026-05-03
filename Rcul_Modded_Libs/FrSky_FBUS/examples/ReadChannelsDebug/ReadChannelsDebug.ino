#include <FrSky_FBUS.h>

static uint32_t lastPrint = 0;

void printStatus() {
  Serial.print("good=");
  Serial.print(FrSky_FBUS.goodFrames());
  Serial.print(" crc=");
  Serial.print(FrSky_FBUS.crcErrors());
  Serial.print(" len=");
  Serial.print(FrSky_FBUS.lengthErrors());
  Serial.print(" type=");
  Serial.print(FrSky_FBUS.typeErrors());
  Serial.print(" status=");
  Serial.print((int)FrSky_FBUS.lastStatus());
  Serial.print(" rawLen=");
  Serial.println(FrSky_FBUS.lastRawFrameLength());
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}

  Serial.println("FrSky FBUS ReadChannelsDebug");
  Serial.println("FBUS doit etre non inverse, 460800 bauds, 8N1.");

  FrSky_FBUS.begin(Serial1);

  // A utiliser uniquement pour debug si le framing semble bon mais que le CRC bloque.
  // FrSky_FBUS.setChecksumRequired(false);
}

void loop() {
  if (FrSky_FBUS.read()) {
    Serial.print("FRAME ");
    Serial.print(FrSky_FBUS.goodFrames());
    Serial.print("  CH=");
    Serial.print(FrSky_FBUS.channelCount());
    Serial.print("  ");

    for (uint8_t i = 0; i < FrSky_FBUS.channelCount(); i++) {
      Serial.print(FrSky_FBUS.channelRaw(i));
      Serial.print('/');
      Serial.print(FrSky_FBUS.channelUs(i));
      Serial.print('\t');
    }

    Serial.print(" RSSI=");
    Serial.print(FrSky_FBUS.rssi());
    Serial.print(" FS=");
    Serial.print(FrSky_FBUS.failsafe());
    Serial.print(" LOST=");
    Serial.println(FrSky_FBUS.frameLost());
  }

  if (millis() - lastPrint > 1000) {
    lastPrint = millis();
    printStatus();
  }
}
