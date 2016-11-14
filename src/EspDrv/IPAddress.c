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

#include "IPAddress.h"

#include <stdlib.h>

bool IPAddress_parse( unsigned char * dotSeparatedString, IPAddress* destination ) {

    uint8_t s[ 4 ];
    uint8_t c;
    uint8_t numChars;
    uint8_t *pc;
    int i;
    uint8_t numFields;

    numFields = 0;
    pc = dotSeparatedString;

    c = *pc;
    while( c != 0 && numFields < 4 )  {
        
        numChars = 0;
        while( isDigit( c ) && numChars < 3 ) {

            s[ numChars++ ] = c;
            pc++;
            c = *pc;

        }

        if ( numChars > 0 && numChars <= 3 ) {
            s[ numChars ] = 0;
            i = atoi( s );
            if ( i <= 255 ) {
                destination[ numFields++ ] = (uint8_t)i;
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }
        
        // Skip '.'
        if ( numFields < 4 && c != '.' ) {
            return false;
        }
        pc++;
        c = *pc;

    }

    return numFields == 4 ? true : false;

}

void IPAddress_copy( IPAddress* origin, IPAddress* destination ) {

    destination[ 0 ] = origin[ 0 ];
    destination[ 1 ] = origin[ 1 ];
    destination[ 2 ] = origin[ 2 ];
    destination[ 3 ] = origin[ 3 ];

}
