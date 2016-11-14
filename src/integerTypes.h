/*
    This file is part of ftpUno

    ftpUno is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INTEGERTYPES_H
#define INTEGERTYPES_H

#include <stdint.h>

typedef u8_t uint8_t;       /* 8 bit type */
typedef u16_t uint16_t;     /* 16 bit type */
typedef u32_t uint32_t;     /* 32 bit type */

typedef i8_t int8_t;        /* 8 bit signed type */
typedef i16_t int16_t;      /* 16 bit signed type */
typedef i32_t int32_t;      /* 32 bit signed type */

typedef uint8_t bool;

#define true (1)
#define false (0)

#ifndef NULL
#define NULL ((void*)0)
#endif

#endif /* INTEGERTYPES_H */

