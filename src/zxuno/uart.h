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

#ifndef ZXUNO_UART_H
#define ZXUNO_UART_H

#include "../integerTypes.h"

#define UART_DATA_REG 0xc6
#define UART_STAT_REG 0xc7
#define UART_BYTE_RECEIVED_BIT 0x80
#define UART_BYTE_TRANSMITTING_BIT 0x40

extern void UART_begin();

extern void UART_writeByte( uint8_t value );
extern uint16_t UART_write( uint8_t* buf, uint16_t len );

extern void UART_print( uint8_t *s );
extern void UART_println( uint8_t *s );

extern int UART_available();
extern uint8_t UART_readBlocking();
extern int UART_read();
extern int UART_read_timeout( long timeout_ms );
extern int UART_peek();

extern int32_t UART_parseInt( long timeout_ms );
extern bool UART_find( uint8_t *s, long timeout_ms );

#endif /* ZXUNO_UART_H */

