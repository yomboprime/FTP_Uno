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

#ifndef TEXTUTILS_H
#define TEXTUTILS_H

#include <stdio.h>
#include "integerTypes.h"

#define COLOR_ATTRIBUTE_START_ADDRESS 22528

#define UDG_GRAPHICS_START 128

extern void textUtils_cls();

extern void textUtils_32ColumnsMode();

extern void textUtils_64ColumnsMode();

extern void textUtils_print( uint8_t *text );
extern void textUtils_print_l( long n );

#define textUtils_println( x ) textUtils_print( x ); fputc_cons( '\n' )
#define textUtils_println_l( x ) textUtils_print_l( x ); fputc_cons( '\n' )

extern void textUtils_printAt32( int x, int y );

extern void textUtils_printAt64( int x, int y );

extern void textUtils_setAttributes( uint8_t attributes );

extern void textUtils_paintCharWithAttributes( uint8_t x, uint8_t y, uint8_t attributes );
extern void textUtils_paintSegmentWithAttributes( uint8_t x0, uint8_t x1, uint8_t y, uint8_t attributes );
extern void textUtils_paintRectangleWithAttributes( uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1, uint8_t attributes );

extern void textUtils_paintSegmentWithBright( uint8_t x0, uint8_t x1, uint8_t y, bool bright );

extern void textUtils_defineUDGGraphic( uint8_t *graphic, uint16_t graphicIndex );

extern bool isDigit( uint8_t c );

#endif /* TEXTUTILS_H */
