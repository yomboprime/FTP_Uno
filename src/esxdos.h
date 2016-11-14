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

#ifndef ESXDOS_H
#define ESXDOS_H

#include "integerTypes.h"

// ESXDOS calls

#define ESXDOS_M_GETSETDRV ( 0x89 )

#define ESXDOS_F_OPEN ( 0x9A )
#define ESXDOS_F_CLOSE ( 0x9B )

#define ESXDOS_F_SYNC ( 0x9C )

#define ESXDOS_F_READ ( 0x9D )
#define ESXDOS_F_WRITE ( 0x9E )

#define ESXDOS_F_SEEK ( 0x9F )
#define ESXDOS_F_GETPOS ( 0xA0 )

#define ESXDOS_F_FSTAT ( 0xA1 )

#define ESXDOS_F_OPENDIR ( 0xA3 )
#define ESXDOS_F_READDIR ( 0xA4 )

#define ESXDOS_F_GETCWD ( 0xA8 )
#define ESXDOS_F_CHDIR ( 0xA9 )

#define ESXDOS_F_UNLINK ( 0xAD )

// File access modes

// Open for read
#define ESXDOS_FILEMODE_READ ( 0x01 )

// Open for write, fail if exists
#define ESXDOS_FILEMODE_WRITE ( 0x06 )

// Open for write, create if not exists, truncate if exists
#define ESXDOS_FILEMODE_WRITE_CREATE ( 0x0E )


// Other definitions

#define ESXDOS_FILE_ATTRIBUTE_DIR_BIT ( 0x10 )

// Struct used in the ESXDOS_fstat function
typedef struct {

    uint8_t drive;
    uint8_t device;
    uint8_t attributes;
    uint32_t date;
    uint32_t fileSize;

} ESXDOS_FSTAT_Struct;

/*
 *
 *  Generic functions
 *
 *
*/
extern uint32_t ESXDOS_fsize( int16_t fhandle );
extern bool ESXDOS_isDirectory( int16_t fhandle );

/*
 * ESXDOS API wrapper functions
 *
 * Note: Check with "iferror {...} else {...}" after every call (Carry flag)
 *
*/
extern int16_t ESXDOS_getDefaultDrive();
extern int16_t ESXDOS_fopen( uint8_t *pathFileName, int16_t mode, int16_t drive );
extern void ESXDOS_fclose( uint16_t fhandle );
extern uint16_t ESXDOS_fread( uint8_t *buffer, uint16_t length, int16_t fhandle );
extern uint16_t ESXDOS_fwrite( uint8_t *buffer, uint16_t length, int16_t fhandle );
extern void ESXDOS_fsync( uint16_t fhandle );
extern uint32_t ESXDOS_fseek( uint32_t n, int16_t mode, int16_t fhandle ); // TODO: Not implemented
extern uint32_t ESXDOS_fgetPos( int16_t fhandle ); // TODO: Not implemented
extern int16_t ESXDOS_fstat( ESXDOS_FSTAT_Struct *infoStruct, int16_t fhandle );
extern int16_t ESXDOS_openDirectory( uint8_t *pathDirName, int16_t drive );
extern int16_t ESXDOS_readDirectory( uint8_t *buffer, int16_t dhandle );
extern void ESXDOS_getCWD( uint8_t *buffer, int16_t drive );
extern void ESXDOS_changeDirectory( uint8_t *pathDirName, int16_t drive );
extern void ESXDOS_delete( uint8_t *pathFileName, int16_t drive );

#endif /* ESXDOS_H */

