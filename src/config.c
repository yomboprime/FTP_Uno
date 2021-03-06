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

#include "config.h"
#include "esxdos.h"

#define CONFIG_STATE_NAME 0
#define CONFIG_STATE_VALUE 1
#define CONFIG_STATE_COMMENT 2

bool isValidNameChar( uint8_t c ) {
    return ( ( c == '_' ) || ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) );
}

int loadConfigFile( uint8_t *configFilePath, void *paramCallback, uint8_t *fileBuffer, uint16_t bufferSize ) {
    
    // See config.h for return codes. If >= 0, it means there was an error in that line number

    int16_t drive;
    int16_t fileHandle;
    uint16_t i, numBytes;
    uint8_t c;
    bool doEnd = false;
    
    int resultCode = CONFIG_OK;
    int lineNumber = 1;

    uint8_t parameterName[ PARAM_NAME_LENGTH + 1 ];
    uint8_t parameterNameLength = 0;
    uint8_t parameterValue[ PARAM_VALUE_LENGTH + 1 ];
    uint8_t parameterValueLength = 0;
            
    uint8_t parseState = CONFIG_STATE_NAME;

    drive = ESXDOS_getDefaultDrive();

    iferror {
        return CONFIG_ERROR_NOTFOUND;
    }

    fileHandle = ESXDOS_fopen( configFilePath, ESXDOS_FILEMODE_READ, drive );
    iferror {
        return CONFIG_ERROR_NOTFOUND;
    }

    while ( doEnd == false ) {

        numBytes = ESXDOS_fread( fileBuffer, bufferSize, fileHandle );

        iferror {
            resultCode = CONFIG_ERROR_READING;
            doEnd = true;
            break;
        }

        if ( numBytes == 0 ) {
            // Reached end of file
            if ( parseState == CONFIG_STATE_VALUE ) {
                // Expose last param
                parameterName[ parameterNameLength ] = 0;
                parameterValue[ parameterValueLength ] = 0;
                paramCallback( parameterName, parameterValue );
            }
            doEnd = true;
            break;
        }
        else {

            for ( i = 0; i < numBytes && doEnd == false; i++ ) {

                c = fileBuffer[ i ];
                
                switch( parseState ) {
                    case CONFIG_STATE_NAME:
                        if ( c == '=' ) {
                            if ( parameterNameLength > 0 ) {
                                parseState = CONFIG_STATE_VALUE;
                                parameterValueLength = 0;
                            }
                            else {
                                resultCode = lineNumber;
                                doEnd = true;
                            }
                        }
                        else if ( c == '#' ) {
                            // Start of comment
                            parseState = CONFIG_STATE_COMMENT;
                        }
                        else if ( c == 13 && parameterNameLength == 0 ) {
                            continue;
                        }
                        else if ( c == 10 && parameterNameLength == 0 ) {
                            lineNumber++;
                            continue;
                        }
                        else if ( isValidNameChar( c ) ) {
                            if ( parameterNameLength < PARAM_NAME_LENGTH ) {
                                parameterName[ parameterNameLength++ ] = c;
                            }
                            else {
                                resultCode = lineNumber;
                                doEnd = true;
                            }
                        }
                        else {
                            resultCode = lineNumber;
                            doEnd = true;
                        }
                        break;
                        
                    case CONFIG_STATE_VALUE:
                        if ( c == 10 ) {
                            // Parameter with value found
                            parameterName[ parameterNameLength ] = 0;
                            parameterValue[ parameterValueLength ] = 0;
                            paramCallback( parameterName, parameterValue );
                            parseState = CONFIG_STATE_NAME;
                            parameterNameLength = 0;
                            lineNumber++;
                        }
                        else if ( c == 13 ) {
                            continue;
                        }
                        else {
                            parameterValue[ parameterValueLength++ ] = c;
                        }
                        break;
                        
                    case CONFIG_STATE_COMMENT:
                        if ( c == 10 ) {
                            parseState = CONFIG_STATE_NAME;
                            lineNumber++;
                        }
                        break;
                }
            }
        }
    }

    ESXDOS_fclose( fileHandle );
    iferror {
        return CONFIG_ERROR_CANTCLOSE;
    }
    
    return resultCode;
    
}

uint8_t configErrorMessage[ 26 ];

uint8_t *configError( int error ) {
    
    if ( error >= 0 ) {
        sprintf( configErrorMessage, "Syntax error in line %d", error );
        return configErrorMessage;
    }
    else {
        switch ( error ) {
            case CONFIG_OK:
                return "";

            case CONFIG_ERROR_NOTFOUND:
                return "Config file not found.";

            case CONFIG_ERROR_READING:
                return "Error reading config file.";

            case CONFIG_ERROR_CANTCLOSE:
                return "Couldn't close config file.";
            
            default:
                return "Unknow error.";
        }
    }
    
}
