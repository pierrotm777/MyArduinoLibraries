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
 * Pixels port to HX8340-B controller, hardware SPI mode, ITDB02-2.2SP
 */

#include "Pixels.h"

#ifndef PIXELS_HX8340_H
#define PIXELS_HX8340_H
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
    void deviceWriteData(uint8_t high, uint8_t low);

    int32_t setRegion(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
    void quickFill(int b, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
    void setFillDirection(uint8_t direction);

    void scrollCmd();

public:
    Pixels() : PixelsBase(176, 220) { // Itead ITDB02-2.2SP as default
        scrollSupported = true;
        setSpiPins(13, 11, 10, 7, 9); // dummy code in PPI case
        setPpiPins(38, 39, 40, 41, 0); // dummy code in SPI case
    }

    Pixels(uint16_t width, uint16_t height) : PixelsBase( width, height) {
        scrollSupported = true;
        setSpiPins(13, 11, 10, 7 ,9); // dummy code in PPI case // uint8_t scl, uint8_t sda, uint8_t cs, uint8_t rst, uint8_t wr
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

    writeCmd(0xC1);
    writeData(0xFF);
    writeData(0x83);
    writeData(0x40);
    writeCmd(0x11);

    delay(100);

    writeCmd(0xCA);
    writeData(0x70);
    writeData(0x00);
    writeData(0xD9);
    writeData(0x01);
    writeData(0x11);

    writeCmd(0xC9);
    writeData(0x90);
    writeData(0x49);
    writeData(0x10);
    writeData(0x28);
    writeData(0x28);
    writeData(0x10);
    writeData(0x00);
    writeData(0x06);

    delay(20);

    writeCmd(0xC2);
    writeData(0x60);
    writeData(0x71);
    writeData(0x01);
    writeData(0x0E);
    writeData(0x05);
    writeData(0x02);
    writeData(0x09);
    writeData(0x31);
    writeData(0x0A);

    writeCmd(0xc3);
    writeData(0x67);
    writeData(0x30);
    writeData(0x61);
    writeData(0x17);
    writeData(0x48);
    writeData(0x07);
    writeData(0x05);
    writeData(0x33);

    delay(10);

    writeCmd(0xB5);
    writeData(0x35);
    writeData(0x20);
    writeData(0x45);

    writeCmd(0xB4);
    writeData(0x33);
    writeData(0x25);
    writeData(0x4c);

    delay(10);

    writeCmd(0x3a);
    writeData(0x05);
    writeCmd(0x29);

    delay(10);

    writeCmd(0x33);
    writeData(0x00);
    writeData(0x00);
    writeData(0x00);
    writeData(0xdc);
    writeData(0x00);
    writeData(0x00);

    writeCmd(0x2a);
    writeData(0x00);
    writeData(0x00);
    writeData(0x00);
    writeData(0xaf);
    writeCmd(0x2b);
    writeData(0x00);
    writeData(0x00);
    writeData(0x00);
    writeData(0xdb);

    writeCmd(0x2c);

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
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
        writeData(hi);writeData(lo);
    }
    for (int32_t i = 0; i < counter % 20; i++) {
        writeData(hi);writeData(lo);
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

void Pixels::deviceWriteData(uint8_t high, uint8_t low) {
    writeData(high);
    writeData(low);
}
#endif
