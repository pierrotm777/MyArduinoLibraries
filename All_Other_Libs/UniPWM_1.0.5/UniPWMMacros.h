
/* 
  Universal Software Controlled PWM function library  for Arduino nano
  
  UniPWM convenience macros
  --------------------------------------------------------------------
  
  Copyright (C) 2014 Bernd Wokoeck
  
  Version history:
  1.0   06/04/2014  created
  1.01  06/28/2014  macros added
  
  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.

**************************************************************/

#ifndef UNIPWMMACROS_H
#define UNIPWMMACROS_H

#include "UniPWM.h"
#include <new.h>

// Phases
// CONST( pwmValue, duration )
#define CONST( a, b ) new UniPWMConst( a, b )
// HOLD( pwmValue )
#define HOLD( a ) new UniPWMConst( a )
// RAMP( pwmValueStart, pwmValueEnd, duration )
#define RAMP( a, b, c ) new UniPWMRamp( a, b, c )
// PAUSE( duration )
#define PAUSE( a ) new UniPWMConst( 0, a )

// Sequence
#define SEQUENCE( a ) UNIPWMPHASE_PTR a[]

// ARRAY
#define ARRAY( a )  a, UNIPWM_COUNT_OF(a)


#endif // UNIPWMMACROS_H

