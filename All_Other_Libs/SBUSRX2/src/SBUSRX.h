/*
   SBUSRX.h
   Brian R Taylor, Simon D. Levy

   Copyright (c) 2016 Bolder Flight Systems

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
   and associated documentation files (the "Software"), to deal in the Software without restriction, 
   including without limitation the rights to use, copy, modify, merge, publish, distribute, 
   sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is 
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or 
   substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
   BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
   DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <stdint.h>

// Your application should implement these functions
extern uint8_t sbusSerialAvailable(void);
extern uint8_t sbusSerialRead(void);

class SBUSRX {

    public:

        SBUSRX(void);

        void handleSerialEvent(uint32_t usec);

        bool gotNewFrame(void);

        void getChannelValues(uint16_t* channels, uint8_t* failsafe, uint16_t* lostFrames);

        void getChannelValuesNormalized(float* calChannels, uint8_t* failsafe, uint16_t* lostFrames);

    private:


        static const uint16_t TIMEOUT = 10000;

        static const uint8_t HEADER      = 0x0F;
        static const uint8_t FOOTER1     = 0x00;
        static const uint8_t FOOTER2     = 0x04;
        static const uint8_t LOST_FRAME  = 0x04;
        static const uint8_t FAILSAFE    = 0x08;

        static const uint8_t PAYLOADSIZE = 24;

        static constexpr float SCALE = 0.00122025625f;
        static constexpr float BIAS  = -1.2098840f;

        uint8_t _fpos;
        uint8_t _payload[PAYLOADSIZE];
        bool    _gotNewFrame;
};
