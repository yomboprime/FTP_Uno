/*--------------------------------------------------------------------
This file is part of the Arduino WiFiEsp library.

The Arduino WiFiEsp library is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

The Arduino WiFiEsp library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with The Arduino WiFiEsp library.  If not, see
<http://www.gnu.org/licenses/>.

Ported to ZX-Uno by yomboprime

--------------------------------------------------------------------*/

#ifndef RingBuffer_h
#define RingBuffer_h

#include "../integerTypes.h"

extern void RingBuffer_create();

extern void RingBuffer_reset();
extern void RingBuffer_init();
extern void RingBuffer_push(char c);
extern int RingBuffer_getPos();
extern uint8_t RingBuffer_endsWith(char* str);
extern void RingBuffer_getStr(char * destination, unsigned int skipChars);
extern void RingBuffer_getStrN(char * destination, unsigned int skipChars, unsigned int num);

#endif
