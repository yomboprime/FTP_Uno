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

#include "RingBuffer.h"

#include <string.h>

#include "../textUtils.h"

#define RING_BUFFER_SIZE 32

unsigned int RingBuffer__size;
// add one char to terminate the string
char RingBuffer_ringBuf[ RING_BUFFER_SIZE + 1 ];
char* RingBuffer_ringBufEnd;
char* RingBuffer_ringBufP;

void RingBuffer_create()
{
    RingBuffer__size = RING_BUFFER_SIZE;

    RingBuffer_ringBufEnd = RingBuffer_ringBuf + RING_BUFFER_SIZE;
    RingBuffer_init();
}

void RingBuffer_reset()
{
    RingBuffer_ringBufP = RingBuffer_ringBuf;
}

void RingBuffer_init()
{
    RingBuffer_ringBufP = RingBuffer_ringBuf;
    memset(RingBuffer_ringBuf, 0, RingBuffer__size+1);
}

void RingBuffer_push(char c)
{
    *RingBuffer_ringBufP = c;
    RingBuffer_ringBufP++;
    if (RingBuffer_ringBufP >= RingBuffer_ringBufEnd)
        RingBuffer_ringBufP = RingBuffer_ringBuf;
}



uint8_t RingBuffer_endsWith(const char* str)
{
    char *p1;
    char *p2;
    char *p;
    int findStrLen = strlen(str);

    // b is the start position into the ring buffer
    char* b = RingBuffer_ringBufP - findStrLen;
    if(b < RingBuffer_ringBuf)
        b = b + RingBuffer__size;

    p1 = (char*)&str[0];
    p2 = p1 + findStrLen;

    for(p=p1; p<p2; p++)
    {
        if(*p != *b)
            return 0;

        b++;
        if (b == RingBuffer_ringBufEnd)
            b=RingBuffer_ringBuf;
    }

    return 1;
}



void RingBuffer_getStr(char * destination, unsigned int skipChars)
{
    int len = RingBuffer_ringBufP - RingBuffer_ringBuf - skipChars;

    // copy buffer to destination string
    strncpy(destination, RingBuffer_ringBuf, len);

    // terminate output string
    //destination[len]=0;
}

void RingBuffer_getStrN(char * destination, unsigned int skipChars, unsigned int num)
{
    int len = RingBuffer_ringBufP - RingBuffer_ringBuf - skipChars;

    if (len>num)
        len=num;

    // copy buffer to destination string
    strncpy(destination, RingBuffer_ringBuf, len);

    // terminate output string
    //destination[len]=0;
}
