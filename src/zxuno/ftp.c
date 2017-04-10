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

#include "ftp.h"
#include <stdio.h>
#include <string.h>
#include "../EspDrv/EspDrv.h"
#include "../esxdos.h"

#include "../textUtils.h"

// Change this from 0 to 1 for debugging
#define DEBUG_FTP 0

#if DEBUG_FTP==1

    #include "../textUtils.h"

    #define DEBUGFTP_print( x ) textUtils_print( x )
    #define DEBUGFTP_println( x ) textUtils_println( x )
    #define DEBUGFTP_print_l( x ) textUtils_print_l( x )
    #define DEBUGFTP_println_l( x ) textUtils_println_l( x )
    #define DEBUGFTP_fputc_cons( x ) fputc_cons( x )

#else

    #define DEBUGFTP_print( x )
    #define DEBUGFTP_println( x )
    #define DEBUGFTP_print_l( x )
    #define DEBUGFTP_println_l( x )
    #define DEBUGFTP_fputc_cons( x )

#endif

// Global variables

// Connection parameters
char *FTP_host;
uint16_t FTP_port;
uint16_t FTP_controlSocket;
uint16_t FTP_dataSocket;
uint8_t *FTP_user;
uint8_t *FTP_password;


// High level functions

/*
 * Set connection parameters for following calls to the functions FTP_downloadFile and FTP_listFiles.
 *
 * Note that the string parameters are not copied, they must remain valid after the call.
 *
 */
void FTP_setConnectionParameters( char *host, uint16_t port, uint16_t controlSocket, uint16_t dataSocket, uint8_t *user, uint8_t *password ) {

    FTP_host = host;
    FTP_port = port;
    FTP_controlSocket = controlSocket;
    FTP_dataSocket = dataSocket;
    FTP_user = user;
    FTP_password = password;

}

/*
 * download a directory listing from a FTP server
 *
 * ftpPath: Path including directory name, in the ftp server
 * buffer: A buffer that will be filled in with the listing entries. Each entry occupies:
 * bytes: (1 + <maxBytesFileName> + 1 + 3 + 4):
 * 1 byte, directory ('>') or file (' ') flag
 * <maxBytesFileName> bytes, file name. No null terminating byte is present. Padded with spaces.
 * 1 byte, filename was trimmed ('.') or not (' ')
 * 3 bytes, extension
 * 4 bytes, file size
 * firstEntry: First entry in the listing that will be copied to the buffer. In other
 * words, 'firstEntry' entries are skipped.
 * maxEntries: Maximum number of entries to be filled in the buffer.
 * numEntries: Returned value on success: number of entries obtained in the buffer.
 * maxBytesFileName: Max num characers for file name exluding extension and dot.
 *
 * return FTP error code (see ftp.h)
 * If no error and more entries exist after the last written entry, FTP_NO_ERROR_MORE_FILES is returned.
 */

uint8_t FTP_listFiles( uint8_t *ftpPath, uint8_t *buffer, uint16_t firstEntry, uint16_t maxEntries, uint16_t *numEntries, uint16_t *numTotalEntries, uint8_t maxBytesFileName ) {

    uint16_t dataServerPort = 0;

    uint16_t bytesAvailable;
    uint16_t bytesRead;

    bool connClosed;
    bool connClosedData;

    uint8_t returnCode;

    uint8_t c;

    uint8_t parseState;
    uint8_t fileOrDirectory;
    bool beganWithSpaces;
    bool parsingFileSize;
    uint32_t fileSize;
    uint8_t numCharsFileName;
    bool lastCharIsPoint;
    uint8_t i;
    uint8_t extension0;
    uint8_t extension1;
    uint8_t extension2;

    long t0;

    // Start control connection
    returnCode = controlConnection3Attempts( &connClosed, &dataServerPort );
    if ( returnCode != FTP_NO_ERROR ) {
        return returnCode;
    }

    // Start data connection
    EspDrv_stopClient( FTP_dataSocket );
    if ( EspDrv_startClient( FTP_host, dataServerPort, FTP_dataSocket, TCP_MODE ) == true ) {

        DEBUGFTP_println("Connected to server data socket.");

    } else {

        DEBUGFTP_println("Can't connect to server data socket.");

        return FTP_ERROR_CANNOT_CONNECT_DATA;

    }

    sprintf( buffer, "CWD %s", ftpPath );
    if ( FTP_sendCommand( buffer ) == false ) {
        DEBUGFTP_println("\nERROR sending CWD");
        return FTP_ERROR_SENDING_COMMAND;
    }
    if ( FTP_parseCommandResponse( &connClosed ) == false ) {
        DEBUGFTP_println("\nERROR reading CWD response");
        return FTP_ERROR_SENDING_COMMAND;
    }

    if ( FTP_sendCommand( "LIST" ) == false ) {
        DEBUGFTP_println("\nERROR sending LIST");
        return FTP_ERROR_SENDING_COMMAND;
    }
    if ( FTP_parseCommandResponse( &connClosed ) == false ) {
        DEBUGFTP_println("\nERROR reading LIST response");
        return FTP_ERROR_SENDING_COMMAND;
    }

    returnCode = FTP_NO_ERROR;

    DEBUGFTP_println( "\nDOWNLOADING DATA..." );

    connClosed = false;
    connClosedData = false;
    bytesRead = 0;
    parseState = 0;
    beganWithSpaces = true;
    parsingFileSize = false;
    *numEntries = 0;
    *numTotalEntries = 0;
    t0 = millis();
    while ( connClosedData == false && ( millis() - t0 < FTP_WAIT ) ) {

        bytesAvailable = EspDrv_availData( FTP_dataSocket );
        if ( bytesAvailable > 0 ) {

            // Read a single byte
            if ( EspDrv_getData( FTP_dataSocket, &c, false, &connClosedData ) == false) {
                DEBUGFTP_println("ERROR***Reading data***");
                return FTP_ERROR_DOWNLOADING;
            }
            else {

                DEBUGFTP_fputc_cons( c );

                switch( parseState ) {
                    case 0:
                        if ( firstEntry == 0 ) {
                            fileOrDirectory = c == 'd' ? '>' : ' ';
                            *buffer++ = fileOrDirectory;
                        }
                        parseState = 1;
                        beganWithSpaces = true;
                        break;

                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    case 6:
                    case 7:
                    case 8:
                        // Skip begginin spaces, following non space characters, and one space separator
                        if ( beganWithSpaces == true ) {
                            if ( c != ' ' ) {
                                beganWithSpaces = false;
                            }
                        }
                        else {
                            if ( c == ' ' ) {
                                parseState++;
                                beganWithSpaces = true;
                            }
                        }
                        break;

                    case 5:
                        // Parse file size
                        if ( parsingFileSize == false ) {
                            // Skip spaces
                            if ( c != ' ' ) {
                                parsingFileSize = true;
                                fileSize = c - '0';
                            }
                        }
                        else {
                            // Parse the decimal file size
                            if ( c == ' ' ) {
                                numCharsFileName = 0;
                                extension0 = ' ';
                                extension1 = ' ';
                                extension2 = ' ';
                                parsingFileSize = false;
                                beganWithSpaces = true;
                                parseState = 6;
                            }
                            else {
                                fileSize = fileSize * 10 + ( c - '0' );
                            }
                        }
                        break;

                    case 9:
                        if ( c == '\n' ) {
                            if ( firstEntry == 0 ) {
                                for ( i = numCharsFileName; i < maxBytesFileName + 1; i++ ) {
                                    *buffer++ = ' ';
                                }

                                // Write extension
                                *buffer++ = extension0;
                                *buffer++ = extension1;
                                *buffer++ = extension2;

                                // Write file size
                                *buffer++ = fileSize & 0xFF;
                                fileSize >>= 8;
                                *buffer++ = fileSize & 0xFF;
                                fileSize >>= 8;
                                *buffer++ = fileSize & 0xFF;
                                fileSize >>= 8;
                                *buffer++ = fileSize & 0xFF;

                                (*numEntries)++;
                            }
                            else {
                                firstEntry--;
                            }
                            (*numTotalEntries)++;
                            parseState = 10;
                        }
                        else {
                            if ( firstEntry == 0 ) {
                                if ( numCharsFileName < maxBytesFileName ) {
                                    if ( c == '.' ) {

                                        if ( numCharsFileName == 0 || lastCharIsPoint == true ) {
                                            *buffer++ = c;
                                            numCharsFileName++;
                                            lastCharIsPoint = true;

                                            // Skip entry named ".", the current directory
                                            //parseState = 12;
                                        }
                                        else {

                                            for ( i = numCharsFileName; i < maxBytesFileName; i++ ) {
                                                *buffer++ = ' ';
                                            }
                                            *buffer++ = ' ';
                                            numCharsFileName = maxBytesFileName + 1;
                                        }
                                    }
                                    else {
                                        *buffer++ = c;
                                        numCharsFileName++;
                                        lastCharIsPoint = false;
                                    }
                                }
                                else if ( numCharsFileName == maxBytesFileName ) {
                                    *buffer++ = c == '.' ? ' ' : '.';
                                    numCharsFileName++;
                                    extension2 = c;
                                }
                                else {
                                    extension0 = extension1;
                                    extension1 = extension2;
                                    extension2 = c;
                                }
                            }
                        }
                        break;

                    case 10:
                        // Read last '\r'
                        if ( *numEntries >= maxEntries ) {
                            parseState = 11;
                        }
                        else {
                            parseState = 0;
                        }
                        break;

                    case 11:
                        // Count lines, incrementing numTotalEntries.
                        // Since there is more output, there are more entries.
                        returnCode = FTP_NO_ERROR_MORE_FILES;
                        if ( c == '\n' ) {
                            (*numTotalEntries)++;
                        }
                        break;

                    case 12:
                        // Skip characters until '\n'
                        if ( c == '\n' ) {
                            (*numTotalEntries)++;
                            parseState = 10;
                        }
                        break;

                    default:
                        break;
                }



                bytesRead++;

            }

            t0 = millis();

        }

    }

    DEBUGFTP_print( "\nFINISHED TRANSFERRING DATA: " );
    DEBUGFTP_print_l( bytesRead );
    DEBUGFTP_print( " bytes were read, connClosedData=" );
    DEBUGFTP_println( connClosedData == true ? "true" : "false" );

    if ( connClosedData == false ) {

        EspDrv_stopClient( FTP_dataSocket );

        connClosed = false;
        if ( FTP_parseCommandResponse( &connClosed ) == false ) {
            DEBUGFTP_println( "\nERROR reading RETR end response" );
        }

    }

    if ( FTP_sendCommand( "QUIT" ) == false ) {
        DEBUGFTP_println( "\nERROR sending QUIT" );
    }
    else if ( FTP_parseCommandResponse( &connClosed ) == false ) {
        DEBUGFTP_println( "\nERROR reading QUIT response" );
    }

    DEBUGFTP_print("Connection ");
    if (connClosed == false) {
        DEBUGFTP_print("not ");
    }
    DEBUGFTP_println("closed.");

    return returnCode;

}

/*
 * Get complete file name and file size for a given directory entry number.
 *
 * ftpPath: Path including directory name, in the ftp server
 * entry: Entry numer to get info from
 * buffer: A buffer that will be filled in with the file name, up to bufferSize bytes
 * bufferSize: max file name length
 * fileSize: Returned file size
 * fileOrDirectory: Returned '>' for directory, else ' '
 *
 * return FTP error code (see ftp.h)
 */
uint8_t FTP_getFileNameAndSize( uint8_t *ftpPath, uint16_t entry, uint8_t *buffer, uint16_t bufferSize, uint32_t *fileSize, uint8_t *fileOrDirectory ) {

    uint16_t dataServerPort = 0;

    uint16_t bytesAvailable;
    uint16_t bytesRead;

    bool connClosed;
    bool connClosedData;

    uint8_t returnCode;

    uint8_t c;

    uint8_t parseState;
    bool beganWithSpaces;
    bool parsingFileSize;
    uint32_t fileSizeTemp;
    
    long t0;

    // Start control connection
    returnCode = controlConnection3Attempts( &connClosed, &dataServerPort );
    if ( returnCode != FTP_NO_ERROR ) {
        return returnCode;
    }

    // Start data connection
    EspDrv_stopClient( FTP_dataSocket );
    if ( EspDrv_startClient( FTP_host, dataServerPort, FTP_dataSocket, TCP_MODE ) == true ) {

        DEBUGFTP_println("Connected to server data socket.");

    } else {

        DEBUGFTP_println("Can't connect to server data socket.");

        return FTP_ERROR_CANNOT_CONNECT_DATA;

    }

    sprintf( buffer, "CWD %s", ftpPath );
    if ( FTP_sendCommand( buffer ) == false ) {
        DEBUGFTP_println("\nERROR sending CWD");
        return FTP_ERROR_SENDING_COMMAND;
    }
    if ( FTP_parseCommandResponse( &connClosed ) == false ) {
        DEBUGFTP_println("\nERROR reading CWD response");
        return FTP_ERROR_SENDING_COMMAND;
    }

    sprintf( buffer, "LIST" );
    if ( FTP_sendCommand( buffer ) == false ) {
        DEBUGFTP_println("\nERROR sending LIST");
        return FTP_ERROR_SENDING_COMMAND;
    }
    if ( FTP_parseCommandResponse( &connClosed ) == false ) {
        DEBUGFTP_println("\nERROR reading LIST response");
        return FTP_ERROR_SENDING_COMMAND;
    }

    returnCode = FTP_ERROR_FILE_NOT_FOUND;

    DEBUGFTP_println( "\nDOWNLOADING DATA..." );

    connClosed = false;
    connClosedData = false;
    bytesRead = 0;
    parseState = entry == 0 ? 0 : 11;
    beganWithSpaces = true;
    parsingFileSize = false;
    fileSizeTemp = 0;
    t0 = millis();
    while ( connClosedData == false && ( millis() - t0 < FTP_WAIT ) ) {

        bytesAvailable = EspDrv_availData( FTP_dataSocket );
        if ( bytesAvailable > 0 ) {

            // Read a single byte.
            if ( EspDrv_getData( FTP_dataSocket, &c, false, &connClosedData ) == false) {
                DEBUGFTP_println("ERROR***Reading data***");
                return FTP_ERROR_DOWNLOADING;
            }
            else {

                DEBUGFTP_fputc_cons( c );

                switch( parseState ) {

                    case 11:
                        // Count lines (entries)
                        if ( c == '\n' ) {
                            parseState = 12;
                        }
                        break;

                    case 12:
                        // Read last '\r'
                        entry--;
                        if ( entry == 0 ) {
                            parseState = 0;
                        }
                        else {
                            parseState = 11;
                        }
                        break;

                    case 0:
                        *fileOrDirectory = c == 'd' ? '>' : ' ';
                        parseState = 1;
                        beganWithSpaces = true;
                        break;

                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    case 6:
                    case 7:
                    case 8:
                        // Skip begginin spaces, following non space characters, and one space separator
                        if ( beganWithSpaces == true ) {
                            if ( c != ' ' ) {
                                beganWithSpaces = false;
                            }
                        }
                        else {
                            if ( c == ' ' ) {
                                parseState++;
                                beganWithSpaces = true;
                            }
                        }
                        break;

                    case 5:
                        // Parse file length
                        if ( parsingFileSize == false ) {
                            // Skip spaces
                            if ( c != ' ' ) {
                                parsingFileSize = true;
                                fileSizeTemp = c - '0';
                            }
                        }
                        else {
                            // Parse the decimal file size
                            if ( c == ' ' ) {

                                *fileSize = fileSizeTemp;
                                beganWithSpaces = true;
                                parseState = 6;
                            }
                            else {
                                fileSizeTemp = fileSizeTemp * 10 + ( c - '0' );
                            }
                        }
                        break;

                    case 9:
                        if ( c == '\n' ) {
                            *buffer = 0;
                            parseState = 10;
                        }
                        else {
                            *buffer = c;
                            if ( bufferSize > 0 ) {
                                bufferSize--;
                                buffer++;
                            }
                        }
                        break;

                    case 10:
                        // Skip al characters
                        returnCode = FTP_NO_ERROR;
                        break;

                    default:
                        break;
                }

                bytesRead++;

            }

            t0 = millis();

        }

    }

    DEBUGFTP_print( "\nFINISHED TRANSFERRING DATA: " );
    DEBUGFTP_print_l( bytesRead );
    DEBUGFTP_print( " bytes were read, connClosedData=" );
    DEBUGFTP_println( connClosedData == true ? "true" : "false" );

    if ( connClosedData == false ) {

        EspDrv_stopClient( FTP_dataSocket );

        connClosed = false;
        if ( FTP_parseCommandResponse( &connClosed ) == false ) {
            DEBUGFTP_println( "\nERROR reading RETR end response" );
        }

    }

    if ( FTP_sendCommand( "QUIT" ) == false ) {
        DEBUGFTP_println( "\nERROR sending QUIT" );
    }
    else if ( FTP_parseCommandResponse( &connClosed ) == false ) {
        DEBUGFTP_println( "\nERROR reading QUIT response" );
    }

    DEBUGFTP_print("Connection ");
    if (connClosed == false) {
        DEBUGFTP_print("not ");
    }
    DEBUGFTP_println("closed.");

    return returnCode;

}

bool FTP_getCWD( uint8_t *path, uint8_t *buffer, uint16_t bufferSize ) {

    bool connClosed = false;
    int bytesWritten;
    uint8_t returnCode = FTP_startControlConnection( &connClosed );

    if ( returnCode != FTP_NO_ERROR ) {

        DEBUGFTP_println("Could not connect to FTP server.");

        return false;

    }

    if ( FTP_sendCommand( "PWD" ) == false ) {
        DEBUGFTP_println( "\nERROR sending PWD" );
        return false;
    }

    if ( FTP_parseCommandResponseBuffer( buffer, bufferSize, &bytesWritten, &connClosed ) == false ) {
        DEBUGFTP_println( "\nERROR reading PWD response" );
        return false;
    }

    if ( FTP_sendCommand( "QUIT" ) == false ) {
        DEBUGFTP_println( "\nERROR sending QUIT" );

    }
    else if ( FTP_parseCommandResponse( &connClosed ) == false ) {
        DEBUGFTP_println( "\nERROR reading QUIT response" );
    }

    DEBUGFTP_print("Connection ");
    if (connClosed == false) {
        DEBUGFTP_print("not ");
    }
    DEBUGFTP_println("closed.");

    // Skip to first "
    while ( *buffer != '"' && bufferSize > 0 ) {
        buffer++;
        bufferSize--;
    }
    if ( bufferSize < 1 ) {
        return false;
    }
    buffer++;
    bufferSize--;

    // Copy path until second "
    while ( *buffer != '"' && bufferSize > 0 ) {
        *path++ = *buffer++;
        bufferSize--;
    }

    *path = 0;

    return true;

}

/*
 * download a file from a FTP server
 *
 * host: Host IP address or domain name
 * port: Usually 21
 * ftpPath: Path including file name, in the ftp server
 * sdPath: Path including the file name, to download to the sd drive
 * buffer: A temp buffer to use in the download, min 64 bytes or ftp or sd path length + 7
 * bufferSize: The size in bytes of the buffer
 * controlSocket: Number from 1 to 4
 * dataSocket: Number from 1 to 4
 *
 * return FTP error code (see ftp.h)
 */
uint8_t FTP_downloadFile( uint8_t *ftpPath, uint8_t *sdPath, uint8_t *buffer, uint16_t bufferSize, void (*progressCallback)() ) {

    uint16_t dataServerPort = 0;

    uint16_t bytesAvailable;
    int32_t totalBytesRead;

    bool connClosed;

    uint8_t drive;
    uint8_t fileHandle;

    int16_t bytesRead;
    uint16_t numBytesReadInBuffer;
    uint16_t bytesAvailableBuffer;
    uint8_t *bufPointer;

    uint8_t returnCode;

    uint8_t c;

    long t0;


    // Open file for writing
    drive = ESXDOS_getDefaultDrive();

    iferror {
        DEBUGFTP_println( "SD card not inserted." );
        return FTP_ERROR_DRIVE_NOT_READY;
    }

    DEBUGFTP_println( "SD card detected." );

    DEBUGFTP_print( "Drive: " );
    DEBUGFTP_println_l( drive );

    // Start control connection
    returnCode = controlConnection3Attempts( &connClosed, &dataServerPort );
    if ( returnCode != FTP_NO_ERROR ) {
        return returnCode;
    }

    // Start data connection
    EspDrv_stopClient( FTP_dataSocket );
    if ( EspDrv_startClient( FTP_host, dataServerPort, FTP_dataSocket, TCP_MODE ) == true ) {

        DEBUGFTP_println("Connected to server data socket.");

    } else {

        DEBUGFTP_println("Can't connect to server data socket.");

        return FTP_ERROR_CANNOT_CONNECT_DATA;
    }

    sprintf( buffer, "RETR %s", ftpPath );
    if ( FTP_sendCommand( buffer ) == false ) {
        DEBUGFTP_println("\nERROR sending RETR");
        return FTP_ERROR_SENDING_COMMAND;
    }
    if ( FTP_parseCommandResponse( &connClosed ) == false ) {
        DEBUGFTP_println("\nERROR reading RETR response");
        return FTP_ERROR_SENDING_COMMAND;
    }

    connClosed = false;

    DEBUGFTP_print( "Opening file: " );
    DEBUGFTP_println( sdPath );

    fileHandle = ESXDOS_fopen( sdPath, ESXDOS_FILEMODE_WRITE_CREATE, drive );
    iferror {
        DEBUGFTP_print( "Could not open file for writing." );
        DEBUGFTP_print( "Error code= " );
        DEBUGFTP_println_l( fileHandle );
        returnCode = FTP_ERROR_CANNOT_OPEN_FILE;
    }
    else {

        DEBUGFTP_println( "\nDOWNLOADING DATA..." );

        if ( progressCallback != NULL ) {
            progressCallback( 0L );
        }

        totalBytesRead = 0;
        numBytesReadInBuffer = 0;
        bytesAvailableBuffer = bufferSize;
        bufPointer = buffer;
        t0 = millis();
        while ( ( millis() - t0 < FTP_WAIT ) ) {

            bytesAvailable = EspDrv_availData( FTP_dataSocket );
            if ( bytesAvailable > 0 ) {

                bytesRead = EspDrv_getDataBuf( FTP_dataSocket, bufPointer, bytesAvailableBuffer );
                if ( bytesRead == -1 ) {
                    DEBUGFTP_println("ERROR***Reading data***");
                    ESXDOS_fclose( fileHandle );
                    return FTP_ERROR_DOWNLOADING;
                }
                else {

                    bufPointer += bytesRead;
                    numBytesReadInBuffer += bytesRead;
                    bytesAvailableBuffer -= bytesRead;
                    totalBytesRead += bytesRead;

                    if  ( numBytesReadInBuffer == bufferSize ) {

                        // Write block of data

                        DEBUGFTP_print( "Num bytes to write= " );
                        DEBUGFTP_println_l( numBytesReadInBuffer );

                        ESXDOS_fwrite( buffer, bufferSize, fileHandle );
                        iferror {
                            DEBUGFTP_println( "Error writing to the file." );
                            ESXDOS_fclose( fileHandle );
                            return FTP_ERROR_WRITING_SD;
                        }
                        else {

                            DEBUGFTP_println( "Write ok." );

                            if ( progressCallback != NULL ) {

                                progressCallback( totalBytesRead );

                            }

                        }

                        bufPointer = buffer;
                        numBytesReadInBuffer = 0;
                        bytesAvailableBuffer = bufferSize;

                    }

                }

                t0 = millis();

            }

        }

        if ( numBytesReadInBuffer > 0 ) {

            DEBUGFTP_print( "Num bytes to write= " );
            DEBUGFTP_println_l( numBytesReadInBuffer );

            ESXDOS_fwrite( buffer, numBytesReadInBuffer, fileHandle );
            iferror {
                DEBUGFTP_println( "Error writing to the file." );
                ESXDOS_fclose( fileHandle );
                return FTP_ERROR_WRITING_SD;
            }
            else {

                DEBUGFTP_println( "Write ok." );

                if ( progressCallback != NULL ) {

                    progressCallback( totalBytesRead );

                }

            }

        }

        DEBUGFTP_print( "\nFINISHED TRANSFERRING DATA: " );
        DEBUGFTP_println_l( totalBytesRead );

    }

    ESXDOS_fsync( fileHandle );

    ESXDOS_fclose( fileHandle );

    iferror {
        DEBUGFTP_println( "Error closing the file." );
    }
    else {
        DEBUGFTP_println( "File closed OK" );
    }

    EspDrv_stopClient( FTP_dataSocket );

//    DEBUGFTP_println( "\nReading RETR end response..." );

    connClosed = false;

//    if ( FTP_parseCommandResponse( &connClosed ) == false ) {
//        DEBUGFTP_println( "\nERROR reading RETR end response" );
//    }

    DEBUGFTP_println( "\nSending QUIT..." );

    if ( FTP_sendCommand( "QUIT" ) == false ) {
        DEBUGFTP_println( "\nERROR sending QUIT" );
    }
    else if ( FTP_parseCommandResponse( &connClosed ) == false ) {
        DEBUGFTP_println( "\nERROR reading QUIT response" );
    }

    DEBUGFTP_print( "Connection " );
    if ( connClosed == false ) {
        DEBUGFTP_print("not ");
    }
    DEBUGFTP_println( "closed." );

    return returnCode;

}




// Low level functions

uint8_t FTP_startControlConnection( bool *connClosed ) {

    uint8_t buffer[ 50 ];

    DEBUGFTP_println( "Trying to connect to server..." );

    EspDrv_stopClient( FTP_controlSocket );
    if ( EspDrv_startClient( FTP_host, FTP_port, FTP_controlSocket, TCP_MODE ) == false ) {

        DEBUGFTP_println("Can't connect to server.");

        return FTP_ERROR_CANNOT_CONNECT;

    }

    *connClosed = false;

    sprintf( buffer, "USER %s", FTP_user );
    if ( FTP_sendCommand( buffer ) == false) {
        DEBUGFTP_println( "\nERROR sending USER" );
        return FTP_ERROR_SENDING_COMMAND;
    }
    if ( FTP_parseCommandResponse( connClosed ) == false ) {
        DEBUGFTP_println( "\nERROR reading USER response" );
        return FTP_ERROR_SENDING_COMMAND;
    }

    sprintf( buffer, "PASS %s", FTP_password );
    if ( FTP_sendCommand( buffer ) == false ) {
        DEBUGFTP_println( "\nERROR sending PASS" );
        return FTP_ERROR_SENDING_COMMAND;
    }
    if ( FTP_parseCommandResponse( connClosed ) == false ) {
        DEBUGFTP_println( "\nERROR reading PASS response" );
        return FTP_ERROR_INVALID_USER;
    }

    if ( FTP_sendCommand( "SYST" ) == false ) {
        DEBUGFTP_println( "\nERROR sending SYST" );
        return FTP_ERROR_SENDING_COMMAND;
    }
    if ( FTP_parseCommandResponse( connClosed ) == false ) {
        DEBUGFTP_println("\nERROR reading SYST response");
        return FTP_ERROR_SENDING_COMMAND;
    }

    if ( FTP_sendCommand( "TYPE I" ) == false ) {
        DEBUGFTP_println( "\nERROR sending TYPE" );
        return FTP_ERROR_SENDING_COMMAND;
    }
    if ( FTP_parseCommandResponse( connClosed ) == false ) {
        DEBUGFTP_println( "\nERROR reading TYPE response" );
        return FTP_ERROR_SENDING_COMMAND;
    }

    return FTP_NO_ERROR;

}

bool FTP_sendCommand( uint8_t *cmd ) {

    DEBUGFTP_print( "\nSending cmd: " );
    DEBUGFTP_println( cmd );

    if ( EspDrv_sendData( FTP_controlSocket, cmd, strlen( cmd ), true) == false ) {
        DEBUGFTP_println( "Error sending FTP command:" );
        DEBUGFTP_println( cmd );
        return false;
    }

    return true;

}

bool FTP_parseCommandResponse( bool *connClosed ) {

    uint8_t numLineTerminatorsLeft = 2;
    uint8_t c;
    bool errorCodeRead = false;
    bool errorCodeOK = false;

    while ( *connClosed == false && numLineTerminatorsLeft > 0 ) {

        if ( EspDrv_availData( FTP_controlSocket ) > 0 ) {

            if ( EspDrv_getData( FTP_controlSocket, &c, false, connClosed ) == false ) {
                DEBUGFTP_println( "ERROR***1***" );
                return false;
            }
            else {

                DEBUGFTP_fputc_cons( c );

                if ( errorCodeRead == false ) {
                    errorCodeOK = c <= '3';
                    errorCodeRead = true;
                }

                if ( c == (numLineTerminatorsLeft & 1 == 1 ? '\r' : '\n' ) ) {
                    numLineTerminatorsLeft--;
                } else {
                    numLineTerminatorsLeft = 2;
                }

            }

        }
    }

    DEBUGFTP_println( "\nCommand response OK." );

    return errorCodeOK;

}

bool FTP_parseCommandResponseBuffer( uint8_t *buffer, uint16_t bufferSize, int *bytesWritten, bool *connClosed ) {

    uint8_t numLineTerminatorsLeft = 2;
    uint8_t c;
    bool errorCodeRead = false;
    bool errorCodeOK = false;
    uint8_t *bufPos = buffer;
    int bytesWrittenTemp = 0;

    *bytesWritten = 0;

    while ( *connClosed == false && numLineTerminatorsLeft > 0 ) {

        if ( EspDrv_availData( FTP_controlSocket ) > 0) {

            if ( EspDrv_getData( FTP_controlSocket, &c, false, connClosed ) == false ) {
                DEBUGFTP_println( "ERROR***2***" );
                return false;
            } else {

                DEBUGFTP_fputc_cons( c );

                if ( bytesWrittenTemp < bufferSize ) {
                    *bufPos++ = c;
                    bytesWrittenTemp++;
                }

                if ( errorCodeRead == false ) {
                    errorCodeOK = c <= '3';
                    errorCodeRead = true;
                }

                if ( c == ( numLineTerminatorsLeft & 1 == 1 ? '\r' : '\n' ) ) {
                    numLineTerminatorsLeft--;
                } else {
                    numLineTerminatorsLeft = 2;
                }

            }

        }
    }

    *bytesWritten = bytesWrittenTemp;

    DEBUGFTP_println( "\nCommand buffer response OK." );

    return errorCodeOK;

}

bool FTP_PASVCommand( uint16_t *dataServerPort, bool *connClosed ) {

    uint8_t buffer[ 128 ];
    int bytesWritten;
    uint8_t *bufferPos;
    uint8_t c;
    uint8_t nCommas;
    uint16_t hiServerPort;
    uint16_t loServerPort;

    if ( FTP_sendCommand( "PASV" ) == false ) {
        DEBUGFTP_println( "\nERROR sending PASV" );
        return false;
    }

    if ( FTP_parseCommandResponseBuffer( buffer, 128, &bytesWritten, connClosed ) == false ) {
        DEBUGFTP_println( "\nERROR reading PASV response (1)" );
        return false;
    }

    DEBUGFTP_print( "\nBuffer read length: " );
    DEBUGFTP_println_l( bytesWritten );

    buffer[ bytesWritten ] = 0;

    // Parse server port. The string is in the form "...(ip0,ip1,ip2,ip3,porthi,portlo)
    bufferPos = buffer;
    c = *bufferPos++;
    nCommas = 0;
    while ( nCommas < 4 ) {
        while ( c != 0 && c != ',' ) {
            c = *bufferPos++;
        }
        if ( c != ',' ) {
            break;
        } else {
            c = *bufferPos++;
            nCommas++;
        }
    }
    if ( nCommas != 4 ) {
        DEBUGFTP_print( "\nERROR reading PASV response (2):" );
        DEBUGFTP_println_l( nCommas );
        return false;
    }

    hiServerPort = 0;
    while ( isDigit( c ) ) {
        hiServerPort = 10 * hiServerPort + c - '0';
        c = *bufferPos++;
    }
    if ( c != ',' ) {
        DEBUGFTP_println( "\nERROR reading PASV response (3)" );
        return false;
    }
    c = *bufferPos++;

    loServerPort = 0;
    while ( isDigit( c ) ) {
        loServerPort = 10 * loServerPort + c - '0';
        c = *bufferPos++;
    }
    if ( c != ')' ) {
        DEBUGFTP_println( "\nERROR reading PASV response (4)" );
        return false;
    }

    *dataServerPort = ( hiServerPort << 8 ) | loServerPort;

    return true;
}

uint8_t controlConnection3Attempts( bool *connClosed, uint16_t *dataServerPort ) {
 
    uint8_t returnCode;
    uint8_t attempts = 0;

    *connClosed = false;
    returnCode = -1;
    while ( returnCode != FTP_NO_ERROR && attempts < 3 ) {
        
        returnCode = FTP_startControlConnection( connClosed );
        
        if ( returnCode != FTP_NO_ERROR ) {
            DEBUGFTP_println("Could not connect to FTP server.");
            attempts++;
        }
        // PASV command to get data server port
        else if ( FTP_PASVCommand( dataServerPort, connClosed ) == false ) {
            DEBUGFTP_println( "\nERROR executing PASV command" );
            attempts++;
            returnCode = FTP_ERROR_SENDING_COMMAND;
        }
    }
    
    return returnCode;
}
