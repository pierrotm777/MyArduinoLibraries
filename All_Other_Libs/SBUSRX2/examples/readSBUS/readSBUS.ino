/*
   readSBUS.ino : simple example of reading from SBUS receiver

   Copyright (C) Simon D. Levy 2018

   This file is part of SBUSRX.

   SBUSRX is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   SBUSRX is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with SBUSRX.  If not, see <http://www.gnu.org/licenses/>.
 */

// Pick one
static const uint16_t SERIAL_MODE = SERIAL_SBUS;            // STM32L4
//static const uint16_t SERIAL_MODE = SERIAL_8E1_RXINV_TXINV; // Teensy 3.0,3.1,3.2
//static const uint16_t SERIAL_MODE = SERIAL_8E2_RXINV_TXINV; // Teensy 3.5,3.6

#include "SBUSRX.h"

// Required for SBUSRX

uint8_t sbusSerialAvailable(void)
{
    return Serial1.available();
}

uint8_t sbusSerialRead(void)
{
    return Serial1.read();
}

SBUSRX rx;

void serialEvent1(void)
{
    rx.handleSerialEvent(micros());
}

void setup() 
{
    // begin the serial port for SBUS
    Serial1.begin(100000, SERIAL_MODE);

    // begin serial-monitor communication
    Serial.begin(115200);
}

static void showaxis(const char * label, float axval)
{
    char tmp[100];
    sprintf(tmp, "%s: %+2.2f  ", label, axval);
    Serial.print(tmp);
}

void loop() 
{
    if (rx.gotNewFrame()) {

        uint8_t failSafe;
        uint16_t lostFrames = 0;
        float channels[16];

        // look for a good SBUS packet from the receiver
        rx.getChannelValuesNormalized(channels, &failSafe, &lostFrames);

        // First five channels (Throttle, Aieleron, Elevator, Rudder, Auxiliary) are enough to see whether it's working
        showaxis("Thr",  channels[0]);
        showaxis("Ael",  channels[1]);
        showaxis("Ele",  channels[2]);
        showaxis("Rud",  channels[3]);
        showaxis("Aux1", channels[4]);
        showaxis("Aux2", channels[5]);
        Serial.print("    Failsafe: ");
        Serial.print(failSafe);
        Serial.print("    Lost frames: ");
        Serial.println(lostFrames);
    }
}

