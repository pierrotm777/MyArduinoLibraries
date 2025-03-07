/*
 * Pixels. Graphics library for TFT displays.
 *
 * Copyright (C) 2012-2014
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


// There are 5 TODO steps:

// 1. TODO replace all occurences of "TEMPLATE" pattern with a controller name (i.e. PIXELS_TEMPLATE_H -> PIXELS_SSD1289_H)

/*
 * Pixels port to TEMPLATE controller
 */

#include "Pixels.h"

#ifndef PIXELS_TEMPLATE_H
#define PIXELS_TEMPLATE_H
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

// 2. TODO Adjust device resolution and default pins below

    Pixels() : PixelsBase(320, 480) {
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

// 3. TODO put device initialization code here
//    The simplest way would be to copy-paste it from a demo application (hopefully) came with the display module.
//    Use existing device driver .h files as a command syntax reference

    chipDeselect();
}

void Pixels::scrollCmd() {
    int16_t s = (orientation > 1 ? deviceHeight - currentScroll : currentScroll) % deviceHeight;
//    int16_t s = (orientation < 2 ? deviceHeight - currentScroll : currentScroll) % deviceHeight;

    // 4. TODO put scrolling command(s) here. It is a rare case the code is given in demo applications or
    //    in alternative open source libraries. A datasheet usually helps.

    //    writeCmd(0x37);
    //    writeData(highByte(s));
    //    writeData(lowByte(s));
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

    // 5. TODO put memory region bounds definition commands here. The code can be taken
    //    from a demo application, supplied with a device.

    // writeCmd(0x2a);
    // writeData(bb.x1>>8);
    // ...

    return (int32_t)(bb.x2 - bb.x1 + 1) * (bb.y2 - bb.y1 + 1);
}
#endif
