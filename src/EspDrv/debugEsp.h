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
--------------------------------------------------------------------*/

#ifndef EspDebug_H
#define EspDebug_H

#include "../textUtils.h"

// Change _ESPLOGLEVEL_ to set tracing and logging verbosity
// 0: DISABLED: no logging
// 1: ERROR: errors
// 2: WARN: errors and warnings
// 3: INFO: errors, warnings and informational (default)
// 4: DEBUG: errors, warnings, informational and debug

#define _ESPLOGLEVEL_ 0

#if _ESPLOGLEVEL_ > 3
    #define LOGDEBUG(x)      textUtils_println(x)
    #define LOGDEBUG0(x)     textUtils_print(x)
    #define LOGDEBUG0c(x)    fputc_cons(x)
    #define LOGDEBUG1(x,y)   textUtils_print(x); textUtils_print(" "); textUtils_print_l(y); textUtils_print("\n")
    #define LOGDEBUG1ss(x,y) textUtils_print(x); textUtils_print(" "); textUtils_println(y)
    #define LOGDEBUG2(x,y,z) textUtils_print(x); textUtils_print(" "); textUtils_print_l(y); textUtils_print(" "); textUtils_println_l(z)
    #define LOGDEBUG2ssi(x,y,z) textUtils_print(x); textUtils_print(" "); textUtils_print(y); textUtils_print(" "); textUtils_println_l(z)
#else
    #define LOGDEBUG(x)
    #define LOGDEBUG0(x)
    #define LOGDEBUG0c(x)
    #define LOGDEBUG1(x,y)
    #define LOGDEBUG1ss(x,y)
    #define LOGDEBUG2(x,y,z)
    #define LOGDEBUG2ssi(x,y,z)
#endif

#if _ESPLOGLEVEL_ > 2
    #define LOGWARN1(x,y)  textUtils_print("[WiFiEsp] "); textUtils_print(x); textUtils_print(" "); textUtils_println(y)
    #define LOGINFO(x)     textUtils_print("[WiFiEsp] "); textUtils_println(x)
    #define LOGINFO1(x,y)  textUtils_print("[WiFiEsp] "); textUtils_print(x); textUtils_print(" "); textUtils_println(y)
    //orig #define LOGERROR1(x,y) if(_ESPLOGLEVEL_>2) { textUtils_print("[WiFiEsp] "); textUtils_print(x); textUtils_print(" "); textUtils_println(y); }
#else
    #define LOGWARN1(x,y)
    #define LOGINFO(x)
    #define LOGINFO1(x,y)
#endif

#if _ESPLOGLEVEL_ > 1
    #define LOGWARN(x)     textUtils_print("[WiFiEsp] "); textUtils_println(x)
#else
    #define LOGWARN(x)
#endif

#if _ESPLOGLEVEL_ > 0
    #define LOGERROR(x)    textUtils_print("[WiFiEsp] "); textUtils_println(x)
#else
    #define LOGERROR(x)
#endif

#define F(x) x
#define PSTR(x) x

#endif
