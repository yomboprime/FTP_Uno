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

#include "textUtils.h"
#include <stdio.h>
#include <spectrum.h>

/*
 * Clears the screen
 */
void textUtils_cls() {

    fputc_cons( 12 );

}

/*
 * Sets 32 column mode
 */
void textUtils_32ColumnsMode() {

    fputc_cons( 1 );
    fputc_cons( 32 );

}

/*
 * Sets 64 column mode
 */
void textUtils_64ColumnsMode() {

    fputc_cons( 1 );
    fputc_cons( 64 );

}

/*
 * Prints asciiz string
 */
void textUtils_print( uint8_t *text ) {

    fputs( text, stdout );

}

/*
 * Prints integer in decimal notation
 */
void textUtils_print_l( long n ) {

    uint8_t s[ 12 ];

    sprintf( s, "%ld", n );

    fputs( s, stdout );

}

/*
 * Locates the cursor in 32 column mode
 * 0 <= x <= 31
 * 0 <= y <= 23
 */
void textUtils_printAt32( int x, int y ) {

    fputc_cons( 22 );
    fputc_cons( (char)( y + 0x20 ) );
    fputc_cons( (char)( x + 0x20 ) );

}

/*
 * Locates the cursor in 64 column mode
 * 0 <= x <= 63
 * 0 <= y <= 23
 */
void textUtils_printAt64( int x, int y ) {

    fputc_cons( 22 );
    fputc_cons( (char)y );
    fputc_cons( (char)x );

}

/*
 * Sets current ink, paper, bright and flash for painting.
 */
extern void textUtils_setAttributes( uint8_t attributes ) {

    uint8_t paper = ( attributes & 0x38 ) >> 3;
    uint8_t ink = attributes & 0x07;
    uint8_t flash = (attributes & 0x80) != 0 ? 1 : 0;
    uint8_t bright = (attributes & 0x40) != 0 ? 1 : 0;

    fputc_cons( 16 ); fputc_cons( ink + 48 );
    fputc_cons( 17 ); fputc_cons( paper + 48 );
    fputc_cons( 18 ); fputc_cons( flash + 48 );
    fputc_cons( 19 ); fputc_cons( bright + 48 );

}

/*
 * Sets attributes of character x,y at screen
 */
void textUtils_paintCharWithAttributes( uint8_t x, uint8_t y, uint8_t attributes ) {

    uint8_t *ptrAttr = ( (uint8_t *)COLOR_ATTRIBUTE_START_ADDRESS );
    ptrAttr += 32 * y + x;

    *ptrAttr = attributes;

}

/*
 * Sets attributes of character segment x0->x1, y at screen
 */
void textUtils_paintSegmentWithAttributes( uint8_t x0, uint8_t x1, uint8_t y, uint8_t attributes ) {

    uint8_t x;
    uint8_t *ptrAttr = ( (uint8_t *)COLOR_ATTRIBUTE_START_ADDRESS );
    ptrAttr += 32 * y + x0;

    for ( x = x0; x <= x1; x++ ) {
        *ptrAttr++ = attributes;
    }

}

/*
 * Sets attributes of rectangle x0,y0 - x1,y1 at screen
 */
void textUtils_paintRectangleWithAttributes( uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1, uint8_t attributes ) {

    uint8_t y;

    for ( y = y0; y <= y1; y++ ) {

        textUtils_paintSegmentWithAttributes( x0, x1, y, attributes );

    }

}

/*
 * Sets or clears bright of character segment x0->x1, y at screen
 */
void textUtils_paintSegmentWithBright( uint8_t x0, uint8_t x1, uint8_t y, bool bright ) {

    // TODO not tested

    uint8_t x;
    uint8_t attr;
    uint8_t *ptrAttr = ( (uint8_t *)COLOR_ATTRIBUTE_START_ADDRESS );
    ptrAttr += 32 * y + x0;

    for ( x = x0; x <= x1; x++ ) {
        attr= *ptrAttr;
        if ( bright == true ) {
            attr |= BRIGHT;
        }
        else {
            attr &= ~BRIGHT;
        }

        *ptrAttr++ = attr;
    }

}

/*
 * Define a UDG 8x8 pixels graphic in memory. You can use it by "print( UDG_GRAPHICS_START + graphicIndex )"
 * graphic: pointer to 8 bytes defining the graphic
 * graphicIndex: 0 to 15
 */
void textUtils_defineUDGGraphic( uint8_t *graphic, uint16_t graphicIndex ) {

    uint8_t i;

    uint8_t *p = 65368;

    p += (uint8_t *)( graphicIndex << 3 );

    for ( i = 0; i < 8; i++ ) {
        *p++ = *graphic++;
    }

}

bool isDigit( uint8_t c ) {
    return c >= '0' && c <= '9';
}
