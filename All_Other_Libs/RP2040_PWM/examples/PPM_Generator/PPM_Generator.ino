#include "RP2040_PWM.h"

#define PPM_PIN 9  // Pin de sortie du signal PPM
#define NUM_CHANNELS 8  // Nombre de canaux PPM
#define PPM_FRAME_LENGTH 22500  // Durée totale d'une frame en µs
#define PPM_PULSE_LENGTH 400  // Durée d'une impulsion en µs

RP2040_PWM *ppm_pwm = NULL;

// Valeurs des canaux (en microsecondes)
int ppm_channels[NUM_CHANNELS] = {1000, 1200, 1400, 1600, 1800, 2000, 1100, 1500};

void generatePPM() {
    static int channel = 0;
    static int elapsedTime = 0;
    
    if (channel == NUM_CHANNELS) {
        int syncTime = PPM_FRAME_LENGTH - elapsedTime;
        ppm_pwm->setPWM(PPM_PIN, syncTime, 50.0); // Large pause pour synchronisation
        elapsedTime = 0;
        channel = 0;
    } else {
        int pulseTime = PPM_PULSE_LENGTH;  // Temps d'impulsion
        int spaceTime = ppm_channels[channel] - PPM_PULSE_LENGTH;  // Temps d'espacement

        ppm_pwm->setPWM(PPM_PIN, pulseTime, 50.0);  // Impulsion
        delayMicroseconds(pulseTime);
        ppm_pwm->setPWM(PPM_PIN, spaceTime, 50.0);  // Espace
        delayMicroseconds(spaceTime);

        elapsedTime += pulseTime + spaceTime;
        channel++;
    }
}

void setup() {
    ppm_pwm = new RP2040_PWM(PPM_PIN, 20000, 50.0);
}

void loop() {
    generatePPM();
}
