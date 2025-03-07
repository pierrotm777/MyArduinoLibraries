/*
 * Pixels. Graphics library for TFT displays.
 *
 * Copyright (C) 2012-2013  Igor Repinetski
 *
 * The code is written in C/C++ for Arduino and can be easily ported to any microcontroller by rewritting the low level pin access functions.
 *
 * Text output methods of the library rely on Pixelmeister's font data format. See: http://pd4ml.com/pixelmeister
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License. To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 * This library includes some code portions and algoritmic ideas derived from works of
 * - Andreas Schiffler -- aschiffler at ferzkopp dot net (SDL_gfx Project)
 * - K. Townsend http://microBuilder.eu (lpc1343codebase Project)
 */

/*
 * Pixels port to ILI9481 controller, PPI16 mode (HY-3.2TFT)
 */

#include "Pixels.h"

#ifndef PIXELS_ILI9481_H
#define PIXELS_ILI9481_H
#define PIXELS_MAIN

#if defined(PIXELS_ANTIALIASING_H)
#define PixelsBase PixelsAntialiased
#endif

class Pixels : public PixelsBase
#if defined(PIXELS_SPISW_H)
                                    , public SPIsw
#elif defined(PIXELS_SPIHW_H)
                                    , public SPIhw
#elif defined(PIXELS_PPI8_H)
                                    , public PPI8
#elif defined(PIXELS_PPI16_H)
                                    , public PPI16
#endif
{
protected:
    void deviceWriteData(uint8_t high, uint8_t low) {
        writeData(high, low);
    }

    int32_t setRegion(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
    void quickFill(int b, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
    void setFillDirection(uint8_t direction);

    void scrollCmd();

public:
    Pixels() : PixelsBase(320, 480) { // ElecFreaks TFT2.2SP shield as default
        scrollSupported = true;
        setSpiPins(4, 3, 7, 5, 6); // dummy code in PPI case
        setPpiPins(38, 39, 40, 41, 0); // dummy code in SPI case
    }

    Pixels(uint16_t width, uint16_t height) : PixelsBase( width, height) {
        scrollSupported = true;
        setSpiPins(4, 3, 7, 5, 6); // dummy code in PPI case
        setPpiPins(38, 39, 40, 41, 0); // dummy code in SPI case
    }

    void init();
};

#if defined(PIXELS_ANTIALIASING_H)
#undef PixelsBase
#endif

void Pixels::init() {

    initInterface();

    chipSelect();

    writeCmd(0x11);
    delay(20);
    writeCmd(0xD0);
    writeData(0x07);
    writeData(0x42);
    writeData(0x18);

    writeCmd(0xD1);
    writeData(0x00);
    writeData(0x07);
    writeData(0x10);

    writeCmd(0xD2);
    writeData(0x01);
    writeData(0x02);

    writeCmd(0xC0);
    writeData(0x10);
    writeData(0x3B);
    writeData(0x00);
    writeData(0x02);
    writeData(0x11);

    writeCmd(0xC5);
    writeData(0x03);

    writeCmd(0xC8);
    writeData(0x00);
    writeData(0x32);
    writeData(0x36);
    writeData(0x45);
    writeData(0x06);
    writeData(0x16);
    writeData(0x37);
    writeData(0x75);
    writeData(0x77);
    writeData(0x54);
    writeData(0x0C);
    writeData(0x00);

    writeCmd(0x36);
    writeData(0x0a);

    writeCmd(0x3A);
    writeData(0x05);

    writeCmd(0xB6);
    writeData(0x00);
    writeData(0x42);
    writeData(0x3B);

    writeCmd(0x33);
    writeData(0x00);
    writeData(0x00);
    writeData(0x01);
    writeData(0xE1);
    writeData(0x00);
    writeData(0x00);

    writeCmd(0x2A);
    writeData(0x00);
    writeData(0x00);
    writeData(0x01);
    writeData(0x3F);

    writeCmd(0x2B);
    writeData(0x00);
    writeData(0x00);
    writeData(0x01);
    writeData(0xDF);

    //----------------Gamma---------------------------------
    writeCmdData(0xf2, 0x08); // 3Gamma Function Disable
    writeCmdData(0x26, 0x01); // Gamma curve

    writeCmd(0xcf);
    writeData(0x00);
    writeData(0x83);
    writeData(0x30);

    writeCmd(0xed);
    writeData(0x64);
    writeData(0x03);
    writeData(0x12);
    writeData(0x81);

    writeCmd(0xe8);
    writeData(0x85);
    writeData(0x01);
    writeData(0x79);

    writeCmd(0xcb);
    writeData(0x39);
    writeData(0x2c);
    writeData(0x00);
    writeData(0x34);
    writeData(0x02);

    writeCmd(0xea);
    writeData(0x00);
    writeData(0x00);

    writeCmd(0x51);
    writeData(0x00);

    writeCmd(0xE0);
    writeData(0x1f);
    writeData(0x1a);
    writeData(0x18);
    writeData(0x0a);
    writeData(0x0f);
    writeData(0x06);
    writeData(0x45);
    writeData(0x87);
    writeData(0x32);
    writeData(0x0a);
    writeData(0x07);
    writeData(0x02);
    writeData(0x07);
    writeData(0x05);
    writeData(0x00);

//    chipDeselect();
//    //Gamma Setting_10323
//    chipSelect();

    writeCmd(0xE1);
    writeData(0x00);
    writeData(0x25);
    writeData(0x27);
    writeData(0x05);
    writeData(0x10);
    writeData(0x09);
    writeData(0x3a);
    writeData(0x78);
    writeData(0x4d);
    writeData(0x05);
    writeData(0x18);
    writeData(0x0d);
    writeData(0x38);
    writeData(0x3a);
    writeData(0x1f);



    delay(120);
    writeCmd(0x29);

    chipDeselect();
}

void Pixels::scrollCmd() {
    int16_t s = (orientation > 1 ? deviceHeight - currentScroll : currentScroll) % deviceHeight;
    writeCmd(0x37);
    writeData(highByte(s));
    writeData(lowByte(s));
}

void Pixels::setFillDirection(uint8_t direction) {
    fillDirection = direction;
}

void Pixels::quickFill (int color, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {

    int32_t counter = setRegion(x1, y1, x2, y2);
    if( counter == 0 ) {
        return;
    }

    registerSelect();

    uint8_t lo = lowByte(color);
    uint8_t hi = highByte(color);

    for (int16_t i = 0; i < counter / 20; i++) {
        writeData(hi, lo);
        writeData(hi, lo);
        writeData(hi, lo);
        writeData(hi, lo);
        writeData(hi, lo);
        writeData(hi, lo);
        writeData(hi, lo);
        writeData(hi, lo);
        writeData(hi, lo);
        writeData(hi, lo);
        writeData(hi, lo);
        writeData(hi, lo);
        writeData(hi, lo);
        writeData(hi, lo);
        writeData(hi, lo);
        writeData(hi, lo);
        writeData(hi, lo);
        writeData(hi, lo);
        writeData(hi, lo);
        writeData(hi, lo);
    }
    for (int32_t i = 0; i < counter % 20; i++) {
        writeData(hi, lo);
    }
}

int32_t Pixels::setRegion(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {

    Bounds bb(x1, y1, x2, y2);
    if( !checkBounds(bb) ) {
        return 0;
    }

    writeCmd(0x2a);
    writeData(bb.x1>>8);
    writeData(bb.x1);
    writeData(bb.x2>>8);
    writeData(bb.x2);
    writeCmd(0x2b);
    writeData(bb.y1>>8);
    writeData(bb.y1);
    writeData(bb.y2>>8);
    writeData(bb.y2);
    writeCmd(0x2c);

    return (int32_t)(bb.x2 - bb.x1 + 1) * (bb.y2 - bb.y1 + 1);
}
#endif
