/*
 * BMP280_Vario.ino — MPX_MSB v2.0.0 + Adafruit BMP280
 * Matériel testé : Multiplex RX-9-DR (M-LINK) + Teensy 4.0 (Serial3 @38400)
 * Câblage BMP280 (I2C) : 3V3, GND, SCL, SDA (adresse 0x76 ou 0x77)
 * Libs requises : Adafruit BMP280 Library (+ Adafruit Unified Sensor)
 *
 * Capteurs exposés à la radio :
 *  - Vbat (addr 3)        -> valeur fixe/dynamique au choix
 *  - Temp1 (addr 6)       -> température BMP280
 *  - Temp2 (addr 7)       -> libre (ex: autre sonde), ici on met une valeur fixe
 *  - Altitude (addr 8)    -> MPX_HEIGHT (1 m)
 *  - Vario (addr 2)       -> MPX_VSPEED (0.1 m/s = *10)
 *  - 4 alarmes digitales (addr 9..12) -> optionnel, exemple inclus
 */

#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <MPX_MSB.h>

using namespace MPX;

Mpx_Msb mpx;
Adafruit_BMP280 bmp;  // I2C

// -------- Réglages BMP280 --------
#define BMP280_ADDR_PRIMARY   0x76
#define BMP280_ADDR_SECONDARY 0x77

// Pression niveau mer pour altitude (hPa). Adapte à ta localisation/conditions.
float SEA_LEVEL_HPA = 1013.25f;

// -------- Exemples de variables "externes" si tu en as déjà --------
// (Sinon tu peux les ignorer et garder des valeurs fixes)
float batteryVoltage = 15.0f; // mets ici ton calcul de tension si besoin
float temp2_demo     = 30.0f; // Temp2 libre

// -------- État pour calcul du vario --------
static uint32_t lastMs = 0;
static float    lastAlt = 0.0f;

void setup() {
  // --- Télémétrie Multiplex ---
  mpx.begin(Serial3, 38400);

  // Déclare ce que tu veux exposer (valeurs initiales)
  mpx.sendVbat(batteryVoltage);  // addr 3
  mpx.sendTmp1(25.0f);           // addr 6 (sera remplacée par BMP temp)
  mpx.sendTmp2(temp2_demo);      // addr 7

  // Mappe ALT et VARIO (choix d’adresses usuelles et non conflictuelles)
  mpx.mapVarioAddrs(/*alt*/8, /*vspd*/2);

  // (Optionnel) alarmes digitales si tu veux les garder
  // mpx.addAlarmDigital(18,  9, MPX_LIQUID);
  // mpx.addAlarmDigital(19, 10, MPX_LIQUID);
  // mpx.addAlarmDigital(22, 11, MPX_LIQUID);
  // mpx.addAlarmDigital(23, 12, MPX_LIQUID);

  // --- BMP280 init ---
  Wire.begin();
  bool ok = bmp.begin(BMP280_ADDR_PRIMARY);
  if (!ok) ok = bmp.begin(BMP280_ADDR_SECONDARY);

  if (!ok) {
    // Si tu veux déboguer : bloquer ici ou continuer sans BMP.
    // while(1);
  } else {
    // Réglages “propres” pour une altitude/vario plus stable
    bmp.setSampling(
      Adafruit_BMP280::MODE_NORMAL,
      Adafruit_BMP280::SAMPLING_X2,   // Temp oversampling
      Adafruit_BMP280::SAMPLING_X16,  // Pressure oversampling
      Adafruit_BMP280::FILTER_X16,    // Filtrage interne
      Adafruit_BMP280::STANDBY_MS_500
    );

    // Premier échantillon pour initialiser le vario
    float tC = bmp.readTemperature();
    float alt = bmp.readAltitude(SEA_LEVEL_HPA);
    lastAlt = isfinite(alt) ? alt : 0.0f;

    // Pousse aussi un premier envoi dans le cache
    if (isfinite(tC))  mpx.sendTmp1(tC);
    mpx.Vario(lastAlt, 0.0f);
  }

  mpx.setIdleMicros(300);
  mpx.setEchoMasking(true);
}

void loop() {
  // --- Lecture BMP280 ---
  float tC   = bmp.readTemperature();              // °C
  float altM = bmp.readAltitude(SEA_LEVEL_HPA);    // m (peut être négatif)

  // --- Mises à jour des valeurs exposées ---
  // Vbat : mets à jour si tu mesures réellement la tension
  mpx.sendVbat(batteryVoltage);

  // Temp1 depuis BMP (si valide)
  if (isfinite(tC)) mpx.sendTmp1(tC);

  // Calcul vario simple (dérivée d'altitude)
  float vspd = 0.0f;
  uint32_t now = millis();
  if (isfinite(altM)) {
    if (lastMs != 0) {
      float dt = (now - lastMs) / 1000.0f;
      if (dt > 0.002f) {
        vspd = (altM - lastAlt) / dt;  // m/s (positif = monte)
      }
    }
    lastAlt = altM;
    lastMs = now;

    // Alimente la partie VARIO (alt + vspd)
    mpx.Vario(altM, vspd);
  }

  // (Optionnel) Temp2 si tu as une autre sonde
  mpx.sendTmp2(temp2_demo);

  // --- Répond aux polls de la radio ---
  mpx.poll();
}
