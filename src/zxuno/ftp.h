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

#ifndef FTP_H
#define FTP_H

#include "../integerTypes.h"

// FTP Wait time ms
#define FTP_WAIT 2000

// Size of an entry in the FTP_listFiles buffer result, minus file name bytes
// 1 byte type + 1 byte separation + 3 bytes extension + 4 bytes file size
#define FTP_DIR_ENTRY_SIZE ( 9 )

// High level functions

#define FTP_NO_ERROR 0
#define FTP_NO_ERROR_MORE_FILES 1
#define FTP_ERROR_CANNOT_CONNECT 2
#define FTP_ERROR_CANNOT_CONNECT_DATA 3
#define FTP_ERROR_INVALID_USER 4
#define FTP_ERROR_SENDING_COMMAND 5
#define FTP_ERROR_DRIVE_NOT_READY 6
#define FTP_ERROR_CANNOT_OPEN_FILE 7
#define FTP_ERROR_WRITING_SD 8
#define FTP_ERROR_DOWNLOADING 9
#define FTP_ERROR_FILE_NOT_FOUND 10

extern void FTP_setConnectionParameters( char *host, uint16_t port, uint16_t controlSocket, uint16_t dataSocket, uint8_t *user, uint8_t *password );
extern uint8_t FTP_listFiles( uint8_t *ftpPath, uint8_t *buffer, uint16_t firstEntry, uint16_t maxEntries, uint16_t *numEntries, uint16_t *numTotalEntries, uint8_t maxBytesFileName );
extern uint8_t FTP_getFileNameAndSize( uint8_t *ftpPath, uint16_t entry, uint8_t *buffer, uint16_t bufferSize, uint32_t *fileSize, uint8_t *fileOrDirectory );
extern uint8_t FTP_downloadFile( uint8_t *ftpPath, uint8_t *sdPath, uint8_t *buffer, uint16_t bufferSize, void (*progressCallback)() );
extern bool FTP_getCWD( uint8_t *path, uint8_t *buffer, uint16_t bufferSize );

// Low level functions
extern uint8_t FTP_startControlConnection( bool *connClosed );
extern bool FTP_sendCommand( uint8_t *cmd );
extern bool FTP_parseCommandResponse( bool *connClosed );
extern bool FTP_parseCommandResponseBuffer( uint8_t *buffer, uint16_t bufferSize, int *bytesWritten, bool *connClosed );
extern bool FTP_PASVCommand( uint16_t *dataServerPort, bool *connClosed);



#endif /* FTP_H */
