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

#ifndef IPADDRESS_H
#define IPADDRESS_H

#include "../integerTypes.h"

// 4 bytes (Z88dk typedef doesn't support pointer types)
typedef uint8_t IPAddress;

extern bool IPAddress_parse( unsigned char * dotSeparatedString, IPAddress* destination );

extern void IPAddress_copy( IPAddress* origin, IPAddress* destination );

#endif /* IPADDRESS_H */

