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

/*
 * Includes
 */
#include <input.h>
#include <stdlib.h>
#include <string.h>
#include <spectrum.h>
#include <math.h>

#include "config.h"
#include "textUtils.h"
#include "zxuno/ftp.h"

#include "EspDrv/EspDrv.h"
#include "esxdos.h"

/*
 * Macros / prototypes
 */
#define getKey() in_Inkey()
uint16_t waitKey();

void loadConfig();
void configCallback( uint8_t *param, uint8_t *value );

void defineGraphics();
void brightSelection( bool bright );

void connectToWiFi();
bool refreshFTPListing();
void showFTPFileInfo();

void initDrive();
void refreshSDListing();
void showSDFileInfo();
bool getSDFileNameAndSize( uint8_t *path, uint32_t *fileSize, uint8_t *fileOrDirectory );

void updateVerticalBar( uint16_t xPos, uint16_t pos, uint16_t total );

void printToStatusBox( uint8_t *line1, uint8_t *line2 );
void printToTitleBox( bool secondColumn, uint8_t *line1, uint8_t *line2 );
void printToBox( uint8_t attributes, uint16_t x, uint16_t y, uint16_t length, uint8_t *line1, uint8_t *line2 );

uint8_t *shortString( uint8_t *string, int maxSize );
bool concatPath( uint8_t *string1, uint8_t *string2, uint16_t maxSize );

void infiniteLoop();
/*
 * UDG Graphics
 */
#define UDG_DIRECTORY 0
#define UDG_BAR0 1
#define UDG_BAR1 2
#define UDG_BAR2 3
#define UDG_BARBACKGROUND0 4
#define UDG_BARBACKGROUND1 5
#define UDG_BARBACKGROUND2 6

static uint8_t directoryUDG [] = {
    0, 126, 130, 159, 161, 161, 194, 254
};

static uint8_t bar0UDG [] = {
    255, 129, 129, 129, 129, 129, 129, 129
};

static uint8_t bar1UDG [] = {
    129, 129, 129, 129, 129, 129, 129, 129
};

static uint8_t bar2UDG [] = {
    129, 129, 129, 129, 129, 129, 129, 255
};

static uint8_t barBackground0UDG [] = {
    170, 0, 129, 0, 129, 0, 129, 0
};

static uint8_t barBackground1UDG [] = {
    129, 0, 129, 0, 129, 0, 129, 0
};

static uint8_t barBackground2UDG [] = {
    129, 0, 129, 0, 129, 0, 129, 84
};

/*
 * GUI elements
*/
// Coordinates
#define X_COORD_TITLE_BOX 0
#define X_SIZE_TITLE_BOX 32
#define X_SIZE_TITLE_BOX1 17
#define X_SIZE_TITLE_BOX2 13
#define Y_COORD_TITLE_BOX 0
#define Y_SIZE_TITLE_BOX 2

#define X_COORD_FTP_LIST 0
#define Y_COORD_FTP_LIST 2
#define X_SIZE_FTP_LIST 16

#define X_COORD_SD_LIST 18
#define Y_COORD_SD_LIST 2
#define X_SIZE_SD_LIST 13

#define X_COORD_STATUS_BOX 0
#define Y_COORD_STATUS_BOX 22
#define X_SIZE_STATUS_BOX 32
#define Y_SIZE_STATUS_BOX 2

#define X_COORD_FTP_VERT_BAR 16
#define X_COORD_SD_VERT_BAR 31
#define Y_COORD_START_VERT_BAR 2
#define Y_COORD_END_VERT_BAR 21

// attributes
#define ATTRIBUTES_TITLE_BOX ( PAPER_BLUE | INK_WHITE | BRIGHT )
#define ATTRIBUTES_LISTING ( PAPER_CYAN | INK_BLACK )
#define ATTRIBUTES_LISTING_SELECTED ( PAPER_CYAN | INK_BLACK | BRIGHT )
#define ATTRIBUTES_STATUS_BOX ( PAPER_BLACK | INK_WHITE | BRIGHT )
#define ATTRIBUTES_VERT_BAR ( PAPER_WHITE | INK_BLACK )
#define ATTRIBUTES_VERT_BAR_BACKGROUND ( PAPER_WHITE | INK_BLACK | BRIGHT )
#define ATTRIBUTES_DIRECTORY_ICON ( PAPER_CYAN | INK_YELLOW )

/*
 * Global variables / other definitions
 * Use of global variables is maximized due to performance.
 */

uint16_t key, key2, i, j, k;
uint32_t longTemp1;
uint16_t diff;
uint32_t fileSize1;
uint8_t fileOrDirectory1;

uint8_t *pbuffer;
uint8_t *pbuffer2;
uint8_t *pbuffer3;

#define MAX_DISPLAY_DIR_ENTRIES 20
#define MAX_DISPLAY_DIR_ENTRIES_2 400
#define MAX_BYTES_FTP_FILENAME 20
#define DISPLAY_BYTES_FTP_FILENAME 11

// 1 attr + 8 name + 1 dot + 3 ext + 1 terminating 0 + 4 date + 4 filesize
#define MAX_SD_ENTRY_SIZE 22

#define FTP_FILE_ENTRY_SIZE ( MAX_BYTES_FTP_FILENAME + FTP_DIR_ENTRY_SIZE )

// Disk buffer // TODO 4096
#define BUFFER_SIZE ( 1024 )
uint8_t *ftpunoBuffer = (uint8_t *)0x6000;

// Current FTP path
#define FTP_PATH_SIZE 512
uint8_t *ftpunoFTPPath = (uint8_t *)0x7000;

// Current SD path
#define SD_PATH_SIZE 512
uint8_t *ftpunoSDPath = (uint8_t *)0x7200;

// Filename scratch buffer
#define FILE_PATH_SIZE 512
uint8_t *ftpunoFilePath = (uint8_t *)0x7400;

// FTP List buffer
#define FTP_LIST_SIZE 582
uint8_t *ftpunoFTPList = (uint8_t *)0x7600;

// SD List buffer
#define SD_LIST_SIZE 442
uint8_t *ftpunoSDList = (uint8_t *)0x7846;

// WiFi connection parameters (this is what is stored in the config file)
uint8_t currentWiFiSSID[ WL_SSID_MAX_LENGTH + 1 ];
#define MAX_PASSWORD_LENGTH 63
uint8_t currentWiFiPassword[ MAX_PASSWORD_LENGTH + 1 ];
#define MAX_HOST_LENGTH 120
uint8_t currentHost[ MAX_HOST_LENGTH + 1 ];
uint16_t currentPort;
#define MAX_FTP_USER_LENGTH 48
uint8_t currentFTPUser[ MAX_FTP_USER_LENGTH + 1 ];
#define MAX_FTP_PASSWORD_LENGTH 48
uint8_t currentFTPPassword[ MAX_FTP_PASSWORD_LENGTH + 1 ];


// Current display status variables
uint16_t firstDisplayedFTPEntry = 0;
uint16_t numDisplayedFTPEntries = 0;
uint16_t numTotalFTPEntries = 0;

uint16_t firstDisplayedSDEntry = 0;
uint16_t numDisplayedSDEntries = 0;
uint16_t numTotalSDEntries = 0;

// Selection
bool selectedFTP_NO_SD = false;
uint16_t selectedDisplayedEntry = 0;

// Connection status
bool connectedToWiFi = false;

// SD card variables
bool SD_initiated = false;
int16_t SD_drive;

// Infinite loop variables
bool showFileInfo;
bool downloadFile;

//*****************************************
//*****************************************
//***************** MAIN ******************
//*****************************************
//*****************************************
void main(void) {

    textUtils_32ColumnsMode();
    textUtils_cls();

    defineGraphics();

    zx_border( INK_BLUE );
    
    loadConfig();

    textUtils_paintRectangleWithAttributes( X_COORD_TITLE_BOX, X_SIZE_TITLE_BOX - 1, Y_COORD_TITLE_BOX, Y_SIZE_TITLE_BOX - 1, ATTRIBUTES_TITLE_BOX );

    textUtils_paintRectangleWithAttributes( X_COORD_FTP_LIST,
            X_COORD_FTP_LIST + X_SIZE_FTP_LIST - 1,
            Y_COORD_FTP_LIST,
            Y_COORD_FTP_LIST + MAX_DISPLAY_DIR_ENTRIES - 1,
            ATTRIBUTES_LISTING );

    textUtils_paintRectangleWithAttributes( X_COORD_SD_LIST,
            X_COORD_SD_LIST + X_SIZE_SD_LIST - 1,
            Y_COORD_SD_LIST,
            Y_COORD_SD_LIST + MAX_DISPLAY_DIR_ENTRIES - 1,
            ATTRIBUTES_LISTING );

    textUtils_paintRectangleWithAttributes( X_COORD_SD_LIST - 1,
            X_COORD_SD_LIST - 1,
            Y_COORD_SD_LIST,
            Y_COORD_SD_LIST + MAX_DISPLAY_DIR_ENTRIES - 1,
            PAPER_BLUE | INK_BLACK );

    textUtils_paintRectangleWithAttributes( X_COORD_STATUS_BOX,
            X_COORD_STATUS_BOX + X_SIZE_STATUS_BOX - 1,
            Y_COORD_STATUS_BOX,
            Y_COORD_STATUS_BOX + Y_SIZE_STATUS_BOX - 1,
            ATTRIBUTES_STATUS_BOX );

    updateVerticalBar( X_COORD_FTP_VERT_BAR, 0, 20 );
    updateVerticalBar( X_COORD_SD_VERT_BAR, 0, 20 );

    initDrive();

    if ( SD_initiated ) {
        
        refreshSDListing();

    }

    connectToWiFi();

    if ( connectedToWiFi ) {

        FTP_setConnectionParameters( currentHost, currentPort, 1, 2, currentFTPUser, currentFTPPassword );

        if ( FTP_getCWD( ftpunoFTPPath, ftpunoBuffer, FTP_PATH_SIZE ) == false ) {
            printToStatusBox( "Failed to get current CWD", NULL );
        }

        if ( refreshFTPListing() == true ) {

            printToStatusBox( "Ready.", NULL );

        }

    }

    brightSelection( true );

    // The loop
    infiniteLoop();

}
//*****************************************
//*****************************************
//*****************************************
//*****************************************
//*****************************************

void loadConfig() {
    
    int error;
    uint8_t *errorTxt;

    *currentWiFiSSID = 0;
    *currentWiFiPassword = 0;
    *currentHost = 0;
    currentPort = 21;
    *currentFTPUser = 0;
    *currentFTPPassword = 0;
    sprintf( ftpunoFTPPath, "/" );
    sprintf( ftpunoSDPath, "/" );

    error = loadConfigFile( "/SYS/CONFIG/FTP.CFG", configCallback, ftpunoBuffer, BUFFER_SIZE );
    
    if ( error == CONFIG_OK ) {

        if ( *currentWiFiSSID == 0 ) {
            error = CONFIG_ERROR_VALIDATION;
            errorTxt = "SSID not set.";
        }
        else if ( *currentWiFiPassword == 0 ) {
            error = CONFIG_ERROR_VALIDATION;
            errorTxt = "WiFi password not set.";
        }
        else if ( *currentHost == 0 ) {
            error = CONFIG_ERROR_VALIDATION;
            errorTxt = "Server not set.";
        }
        else if ( *currentHost == 0 ) {
            error = CONFIG_ERROR_VALIDATION;
            errorTxt = "Server not set.";
        }
        else if ( *currentFTPUser == 0 ) {
            error = CONFIG_ERROR_VALIDATION;
            errorTxt = "FTP user not set.";
        }
        else if ( *currentFTPPassword == 0 ) {
            error = CONFIG_ERROR_VALIDATION;
            errorTxt = "FTP password not set.";
        }
    }

    if ( error != CONFIG_OK ) {
        
        textUtils_println( "Error loading config file: " );
        textUtils_println( configError( error ) );

        while (1);
        
    }

}

void configCallback( uint8_t *param, uint8_t *value ) {

    if ( strcmp( "ssid", param ) == 0 ) {
        strcpy( currentWiFiSSID, value );
    }
    else if ( strcmp( "wifi_password", param ) == 0 ) {
        strcpy( currentWiFiPassword, value );
    }
    else if ( strcmp( "server", param ) == 0 ) {
        strcpy( currentHost, value );
    }
    else if ( strcmp( "port", param ) == 0 ) {
        currentPort = atoi( value );
    }
    else if ( strcmp( "user", param ) == 0 ) {
        strcpy( currentFTPUser, value );
    }
    else if ( strcmp( "ftp_password", param ) == 0 ) {
        strcpy( currentFTPPassword, value );
    }
    else if ( strcmp( "ftp_initial_dir", param ) == 0 ) {
        strcpy( ftpunoFTPPath, value );
    }
    else if ( strcmp( "sd_initial_dir", param ) == 0 ) {
        strcpy( ftpunoSDPath, value );
    }

}

uint16_t waitKey() {

    uint16_t count = 350;

    k = getKey();
    while ( k > 0 && count > 0 ) {
        k = getKey();
        count--;
    }

    while ( k == 0 ) {
        k = getKey();
    };

    return k;
}

void defineGraphics() {

    textUtils_defineUDGGraphic( directoryUDG, UDG_DIRECTORY );

    textUtils_defineUDGGraphic( bar0UDG, UDG_BAR0 );
    textUtils_defineUDGGraphic( bar1UDG, UDG_BAR1 );
    textUtils_defineUDGGraphic( bar2UDG, UDG_BAR2 );

    textUtils_defineUDGGraphic( barBackground0UDG, UDG_BARBACKGROUND0 );
    textUtils_defineUDGGraphic( barBackground1UDG, UDG_BARBACKGROUND1 );
    textUtils_defineUDGGraphic( barBackground2UDG, UDG_BARBACKGROUND2 );

}

void brightSelection( bool bright ) {

    uint8_t x0 = selectedFTP_NO_SD == true ? X_COORD_FTP_LIST : X_COORD_SD_LIST;
    uint8_t s0 = selectedFTP_NO_SD == true ? X_COORD_FTP_VERT_BAR : X_SIZE_TITLE_BOX2;

    textUtils_paintSegmentWithBright( x0, x0 + s0 - 1, Y_COORD_FTP_LIST + selectedDisplayedEntry, bright );

}

bool refreshFTPListing() {

    bool flagMoreChars;
    uint8_t currentFileNameSize;
    uint8_t result;

    printToStatusBox( "Reading FTP file list...", NULL );

    result = FTP_listFiles( ftpunoFTPPath, ftpunoFTPList, firstDisplayedFTPEntry, MAX_DISPLAY_DIR_ENTRIES, &numDisplayedFTPEntries, &numTotalFTPEntries, MAX_BYTES_FTP_FILENAME );

    if ( result != FTP_NO_ERROR && result != FTP_NO_ERROR_MORE_FILES ) {
        printToStatusBox( "Error retrieving FTP dir list.", NULL );
        return false;
    }

    pbuffer = ftpunoFTPList;

    textUtils_setAttributes( ATTRIBUTES_LISTING );
    for ( i = 0; i < numDisplayedFTPEntries; i++ ) {

        pbuffer2 = pbuffer;

        textUtils_printAt32( X_COORD_FTP_LIST, Y_COORD_FTP_LIST + i );

        // Print directory icon or space
        if ( *pbuffer2++ == '>' ) {
            textUtils_setAttributes( ATTRIBUTES_DIRECTORY_ICON );
            fputc_cons( UDG_GRAPHICS_START + UDG_DIRECTORY );
            textUtils_setAttributes( ATTRIBUTES_LISTING );
        }
        else {
            fputc_cons( ' ' );
        }

        currentFileNameSize = DISPLAY_BYTES_FTP_FILENAME;
        flagMoreChars = pbuffer2[ MAX_BYTES_FTP_FILENAME ] == '.' ? true: false;
        if ( flagMoreChars == true ) {
            currentFileNameSize -= 2;
        }

        for ( j = 0; j < currentFileNameSize; j++ ) {

            fputc_cons( *pbuffer2++ );

        }

        for ( ; j < MAX_BYTES_FTP_FILENAME; j++ ) {

            pbuffer2++;

        }

        // Skip separation char
        pbuffer2++;

        // Print separation
        if ( flagMoreChars == true ) {

            fputc_cons( '.' );
            fputc_cons( '.' );
            fputc_cons( '.' );

        }
        else {

            fputc_cons( ' ' );

        }

        // Extension
        for ( j = 0; j < 3; j++ ) {

            fputc_cons( *pbuffer2++ );

        }

        pbuffer += FTP_FILE_ENTRY_SIZE;

    }

    // Erase rest of lines
    k = X_SIZE_FTP_LIST;
    for ( ; i < MAX_DISPLAY_DIR_ENTRIES; i++ ) {
        textUtils_printAt32( X_COORD_FTP_LIST, Y_COORD_FTP_LIST + i );
        for ( j = 0; j < k; j++ ) {
            fputc_cons( ' ' );
        }
    }

    updateVerticalBar( X_COORD_FTP_VERT_BAR, firstDisplayedFTPEntry, numTotalFTPEntries );
    
    printToTitleBox( false, shortString( currentHost, X_SIZE_TITLE_BOX1 ), shortString( ftpunoFTPPath, X_SIZE_TITLE_BOX1 ) );

    return true;

}

void updateVerticalBar( uint16_t xPos, uint16_t pos, uint16_t total ) {

    // 0 <= pos <= 1

    uint16_t posChars = 0;
    uint16_t sizeChars = MAX_DISPLAY_DIR_ENTRIES;
    uint16_t end;
    float totalInverse;
    float fraction;

    if ( total > MAX_DISPLAY_DIR_ENTRIES ) {

        totalInverse = (int32_t)total;
        totalInverse = 1.0 / totalInverse;
        fraction = ((int32_t)pos);
        fraction *= totalInverse;

        posChars = (uint16_t)( fraction * ((float)MAX_DISPLAY_DIR_ENTRIES) );
        sizeChars = (uint16_t)( ((float)MAX_DISPLAY_DIR_ENTRIES_2 ) * totalInverse );
        if ( sizeChars == 0 ) {
            sizeChars = 1;
        }

    }

    // Adjust to the end
    end = posChars + sizeChars;
    if ( end > MAX_DISPLAY_DIR_ENTRIES ) {
        posChars -= end - MAX_DISPLAY_DIR_ENTRIES;
    }

    // Print bar background
    textUtils_setAttributes( ATTRIBUTES_VERT_BAR_BACKGROUND );
    textUtils_printAt32( xPos, Y_COORD_START_VERT_BAR );
    fputc_cons( UDG_GRAPHICS_START + UDG_BARBACKGROUND0 );
    for ( i = Y_COORD_START_VERT_BAR + 1; i < Y_COORD_END_VERT_BAR; i++ ) {
        textUtils_printAt32( xPos, i );
        fputc_cons( UDG_GRAPHICS_START + UDG_BARBACKGROUND1 );
    }
    textUtils_printAt32( xPos, Y_COORD_END_VERT_BAR );
    fputc_cons( UDG_GRAPHICS_START + UDG_BARBACKGROUND2 );

    // Print bar
    textUtils_setAttributes( ATTRIBUTES_VERT_BAR );
    textUtils_printAt32( xPos, Y_COORD_START_VERT_BAR + posChars );
    fputc_cons( UDG_GRAPHICS_START + UDG_BAR0 );
    for ( i = Y_COORD_START_VERT_BAR + posChars + 1; i < Y_COORD_START_VERT_BAR + posChars + sizeChars - 1; i++ ) {
        textUtils_printAt32( xPos, i );
        fputc_cons( UDG_GRAPHICS_START + UDG_BAR1 );
    }
    textUtils_printAt32( xPos, Y_COORD_START_VERT_BAR + posChars + sizeChars - 1 );
    fputc_cons( UDG_GRAPHICS_START + UDG_BAR2 );

}

void printToStatusBox( uint8_t *line1, uint8_t *line2 ) {
    printToBox( ATTRIBUTES_STATUS_BOX, X_COORD_STATUS_BOX, Y_COORD_STATUS_BOX, X_SIZE_STATUS_BOX, line1, line2 );
}

void printToTitleBox( bool secondColumn, uint8_t *line1, uint8_t *line2 ) {
    printToBox( ATTRIBUTES_TITLE_BOX,
            secondColumn == false ? X_COORD_TITLE_BOX : X_COORD_SD_LIST,
            Y_COORD_TITLE_BOX,
            secondColumn == false ? X_SIZE_TITLE_BOX1 : X_SIZE_TITLE_BOX2,
            line1, line2 );
}

void printToBox( uint8_t attributes, uint16_t x, uint16_t y, uint16_t length, uint8_t *line1, uint8_t *line2 ) {

    textUtils_setAttributes( attributes );

    textUtils_printAt32( x, y );
    i = 0;
    if ( line1 != NULL ) {
        while ( i < length && *line1 > 0 ) {
            fputc_cons( *line1++ );
            i++;
        }
    }
    for ( ; i < length; i++ ) {
        fputc_cons( ' ' );
    }

    textUtils_printAt32( x, y + 1 );
    i = 0;
    if ( line2 != NULL ) {
        while ( i < length && *line2 > 0 ) {
            fputc_cons( *line2++ );
            i++;
        }
    }
    for ( ; i < length; i++ ) {
        fputc_cons( ' ' );
    }

}

void connectToWiFi() {

    uint8_t flag = 0;
    connectedToWiFi = false;

    printToStatusBox( "Initiating WiFi chip...", NULL );

    if ( EspDrv_wifiDriverInit() != true ) {

        printToStatusBox( "Can't talk with the WiFi module.", NULL );

        return;

    }

    printToStatusBox( "Trying to connect to net... ", currentWiFiSSID );

    while ( connectedToWiFi == false ) {

        connectedToWiFi = EspDrv_wifiConnect( currentWiFiSSID, currentWiFiPassword );

        if ( flag == 0 ) {
            printToStatusBox( "Trying to connect to net:-_-_-", currentWiFiSSID );
            flag = 1;
        }
        else{
            printToStatusBox( "Trying to connect to net:_-_-_", currentWiFiSSID );
            flag = 0;
        }

    }

    printToStatusBox( "Connected to WiFi.", NULL );

}

void initDrive() {

    printToStatusBox( "Initiating SD card...", NULL );

    SD_initiated = false;

    SD_drive = ESXDOS_getDefaultDrive();
    iferror {
        printToStatusBox( "SD card not inserted.", NULL );
        return;
    }

    printToStatusBox( "SD card detected.", NULL );

    ESXDOS_getCWD( ftpunoSDPath, SD_drive );
    iferror {
        printToStatusBox( "Error getting SD CWD.", NULL );
        return;
    }

    SD_initiated = true;

}

void refreshSDListing() {

    int16_t dhandle;

    uint16_t readResult;

    uint16_t firstEntryCount;

    uint8_t attributes;


    numDisplayedSDEntries = 0;
    numTotalSDEntries = 0;

    dhandle = ESXDOS_openDirectory( ftpunoSDPath, SD_drive );
    iferror {
        sprintf( ftpunoFilePath, "Error code= %d", dhandle );
        printToStatusBox( "Could not open directory.", ftpunoFilePath );
        return;
    }

    pbuffer = ftpunoSDList;
    firstEntryCount = firstDisplayedSDEntry;

    readResult = 1;
    while ( readResult > 0 ) {

        readResult = ESXDOS_readDirectory( pbuffer, dhandle );
        iferror {
            sprintf( ftpunoFilePath, "Error code= %d", readResult );
            printToStatusBox( "Error reading the directory." , ftpunoFilePath );
            readResult = 0;
        }
        else {

            if ( readResult == 0 ) {
                break;
            }

            if ( numDisplayedSDEntries < MAX_DISPLAY_DIR_ENTRIES ) {

                if ( firstEntryCount == 0 ) {
                    pbuffer += MAX_SD_ENTRY_SIZE;
                    numDisplayedSDEntries++;
                }
                else {
                    firstEntryCount--;
                }

            }

            numTotalSDEntries++;

        }

    }

    pbuffer = ftpunoSDList;
    textUtils_setAttributes( ATTRIBUTES_LISTING );
    for ( i = 0; i < numDisplayedSDEntries; i++ ) {

        pbuffer2 = pbuffer;

        textUtils_printAt32( X_COORD_SD_LIST, Y_COORD_SD_LIST + i );

        // Read attributes byte
        attributes = *pbuffer2++;

        if ( attributes == ESXDOS_FILE_ATTRIBUTE_DIR_BIT ) {
            textUtils_setAttributes( ATTRIBUTES_DIRECTORY_ICON );
            fputc_cons( UDG_GRAPHICS_START + UDG_DIRECTORY );
            textUtils_setAttributes( ATTRIBUTES_LISTING );
        }
        else {
            fputc_cons( ' ' );
        }

        // Print file name
        j = 0;
        while( *pbuffer2 != 0 ) {

            fputc_cons( *pbuffer2++ );
            j++;

        }

        for ( ; j < 12; j++ ) {

            fputc_cons( ' ' );

        }

        pbuffer += MAX_SD_ENTRY_SIZE;

    }

    // Erase rest of lines
    k = X_SIZE_SD_LIST;
    for ( ; i < MAX_DISPLAY_DIR_ENTRIES; i++ ) {
        textUtils_printAt32( X_COORD_SD_LIST, Y_COORD_SD_LIST + i );
        for ( j = 0; j < k; j++ ) {
            fputc_cons( ' ' );
        }
    }

    ESXDOS_fclose( dhandle );

    updateVerticalBar( X_COORD_SD_VERT_BAR, firstDisplayedSDEntry, numTotalSDEntries );

    printToTitleBox( true, "Drive:", shortString( ftpunoSDPath, X_SIZE_TITLE_BOX2 ) );

}

void showFTPFileInfo() {

    if ( selectedDisplayedEntry >= numDisplayedFTPEntries ) {
        return;
    }

    pbuffer = ftpunoFTPList;
    pbuffer += selectedDisplayedEntry * FTP_FILE_ENTRY_SIZE;

    // Type byte
    i = *pbuffer++;

    // Store name
    pbuffer2 = ftpunoFilePath;
    j = 0;
    while ( j < MAX_BYTES_FTP_FILENAME ) {
        *pbuffer2++ = *pbuffer++;
        j++;
    }
    
    // Skip separation char
    if ( *pbuffer++ == '.' ) {
        *pbuffer2++ = '.';
        *pbuffer2++ = '.';
        *pbuffer2++ = '.';
    }
    else if ( *pbuffer != ' ' ) {
        // Dot, if there's extension
        *pbuffer2++ = '.';
    }
    
    // Store extension
    *pbuffer2++ = *pbuffer++;
    *pbuffer2++ = *pbuffer++;
    *pbuffer2++ = *pbuffer++;

    *pbuffer2++ = 0;

    if ( i == '>' ) {
        sprintf( pbuffer2, "<DIR>" );
    }
    else {
        // Store file size
        longTemp1 = *pbuffer++;
        longTemp1 += ( (uint32_t)( *pbuffer++ ) ) << 8;
        longTemp1 += ( (uint32_t)( *pbuffer++ ) ) << 16;
        longTemp1 += ( (uint32_t)( *pbuffer++ ) ) << 24;
        sprintf( pbuffer2, "%ld bytes.", longTemp1 );
    }

    printToStatusBox( ftpunoFilePath, pbuffer2 );

}

void showSDFileInfo() {

    if ( selectedDisplayedEntry >= numDisplayedSDEntries ) {
        return;
    }

    pbuffer = ftpunoSDList;
    pbuffer += selectedDisplayedEntry * MAX_SD_ENTRY_SIZE;

    // Attributes byte
    i = *pbuffer++;

    // Store name
    pbuffer2 = ftpunoFilePath;
    while( *pbuffer > 0 ) {
        *pbuffer2++ = *pbuffer++;
        //j++;
    }
    *pbuffer2++ = 0;
    pbuffer++;

    // Skip date
    pbuffer+= 4;

    if ( i == ESXDOS_FILE_ATTRIBUTE_DIR_BIT ) {
        sprintf( pbuffer2, "<DIR>" );
    }
    else {
        // Store file size
        longTemp1 = *pbuffer++;
        longTemp1 += ( (uint32_t)( *pbuffer++ ) ) << 8;
        longTemp1 += ( (uint32_t)( *pbuffer++ ) ) << 16;
        longTemp1 += ( (uint32_t)( *pbuffer++ ) ) << 24;
        sprintf( pbuffer2, "%ld bytes.", longTemp1 );
    }

    printToStatusBox( ftpunoFilePath, pbuffer2 );

}

bool getSDFileNameAndSize( uint8_t *path, uint32_t *fileSize, uint8_t *fileOrDirectory ) {

    if ( selectedDisplayedEntry >= numDisplayedSDEntries ) {
        return false;
    }

    pbuffer = ftpunoSDList;
    pbuffer += selectedDisplayedEntry * MAX_SD_ENTRY_SIZE;

    // Attributes byte
    *fileOrDirectory = *pbuffer++;

    // Store name
    while( *pbuffer > 0 ) {
        *path++ = *pbuffer++;
    }
    *path = 0;
    pbuffer++;

    // Skip date
    pbuffer+= 4;

    if ( *fileOrDirectory != ESXDOS_FILE_ATTRIBUTE_DIR_BIT ) {
        // Store file size
        longTemp1 = *pbuffer++;
        longTemp1 += ( (uint32_t)( *pbuffer++ ) ) << 8;
        longTemp1 += ( (uint32_t)( *pbuffer++ ) ) << 16;
        longTemp1 += ( (uint32_t)( *pbuffer++ ) ) << 24;
    }

    return true;
}

bool concatPath( uint8_t *string1, uint8_t *string2, uint16_t maxSize ) {

    uint16_t l1 = strlen( string1 );
    uint16_t l2 = strlen( string2 );

    if ( l1 + l2 + 1 > maxSize ) {
        return false;
    }

    string1 += l1;

    while ( l2 > 0 ) {
        *string1++ = *string2++;
        l2--;
    }
    *string1++='/';
    *string1 = 0;

    return true;

}

void pathUpOneDir( uint8_t *path ) {

    pbuffer = path;
    while ( *pbuffer > 0 ) {
        pbuffer++;
    }
    pbuffer--;
    if ( *pbuffer == '/' ) {
        pbuffer--;
    }

    while( pbuffer > path && *pbuffer != '/' ) {
        pbuffer--;
    }

    if ( *pbuffer == '/' ) {
        pbuffer++;
        *pbuffer = 0;
    }

}

uint8_t *shortString( uint8_t *string, int maxSize ) {

    int s = strlen( string );
    if ( s > maxSize ) {
        s -= maxSize;
        string += s;
    }

    return string;

}

void downloadProgressCallback( int32_t totalBytesRead ) {

    sprintf( ftpunoBuffer, "%ld of %ld bytes.", totalBytesRead, fileSize1 );
    printToStatusBox( "Downloading...", ftpunoBuffer );
    
}

void infiniteLoop() {

    while ( true ) {

        // Keyboard input
        key = waitKey();

        showFileInfo = false;

        switch ( key ) {
            // Up
            case 11:
                if ( selectedDisplayedEntry == 0 ) {

                    if ( selectedFTP_NO_SD == true ) {

                        if ( firstDisplayedFTPEntry > 0 ) {

                            diff = MAX_DISPLAY_DIR_ENTRIES;
                            if ( diff > firstDisplayedFTPEntry ) {
                                diff = firstDisplayedFTPEntry;
                            }
                            firstDisplayedFTPEntry -= diff;

                            brightSelection( false );

                            refreshFTPListing();

                            selectedDisplayedEntry = numDisplayedFTPEntries - 1;

                            brightSelection( true );

                        }

                    }
                    else {

                        if ( firstDisplayedSDEntry > 0 ) {

                            diff = MAX_DISPLAY_DIR_ENTRIES;
                            if ( diff > firstDisplayedSDEntry ) {
                                diff = firstDisplayedSDEntry;
                            }
                            firstDisplayedSDEntry -= diff;

                            brightSelection( false );

                            refreshSDListing();

                            selectedDisplayedEntry = numDisplayedSDEntries - 1;

                            brightSelection( true );

                        }

                    }
                }
                else {
                    brightSelection( false );
                    selectedDisplayedEntry--;
                    diff = selectedFTP_NO_SD == true ? numDisplayedFTPEntries : numDisplayedSDEntries;
                    if ( diff > 0 && selectedDisplayedEntry > diff - 1 ) {
                        selectedDisplayedEntry = diff - 1;
                    }
                    brightSelection( true );
                }
                showFileInfo = true;
                break;

            // Down
            case 10:
                if ( selectedDisplayedEntry == MAX_DISPLAY_DIR_ENTRIES - 1 ) {

                    if ( selectedFTP_NO_SD == true ) {

                        if ( firstDisplayedFTPEntry + MAX_DISPLAY_DIR_ENTRIES < numTotalFTPEntries ) {

                            firstDisplayedFTPEntry += MAX_DISPLAY_DIR_ENTRIES;

                            refreshFTPListing();

                            brightSelection( false );

                            selectedDisplayedEntry = 0;

                            brightSelection( true );

                        }

                    }
                    else {

                        if ( firstDisplayedSDEntry + MAX_DISPLAY_DIR_ENTRIES < numTotalSDEntries ) {

                            firstDisplayedSDEntry += MAX_DISPLAY_DIR_ENTRIES;

                            refreshSDListing();

                            brightSelection( false );

                            selectedDisplayedEntry = 0;

                            brightSelection( true );

                        }

                    }
                }
                else {
                    brightSelection( false );
                    selectedDisplayedEntry++;
                    diff = selectedFTP_NO_SD == true ? numDisplayedFTPEntries : numDisplayedSDEntries;
                    if ( diff > 0 && selectedDisplayedEntry > diff - 1 ) {
                        selectedDisplayedEntry = diff - 1;
                    }
                    brightSelection( true );
                }
                showFileInfo = true;
                break;

            // Left and right
            case 8:
            case 9:
                brightSelection( false );
                selectedFTP_NO_SD = selectedFTP_NO_SD == true ? false : true;
                diff = selectedFTP_NO_SD == true ? numDisplayedFTPEntries : numDisplayedSDEntries;
                if ( diff > 0 && selectedDisplayedEntry > diff - 1 ) {
                    selectedDisplayedEntry = diff - 1;
                }
                brightSelection( true );
                showFileInfo = true;
                break;

            // Intro pressed
            case 13:
                if ( selectedFTP_NO_SD == true ) {
                    printToStatusBox( "Obtaining file info...", NULL );
                    diff = FTP_getFileNameAndSize( ftpunoFTPPath,
                            firstDisplayedFTPEntry + selectedDisplayedEntry,
                            ftpunoBuffer, FILE_PATH_SIZE, &fileSize1, &fileOrDirectory1 );
                    if ( diff != FTP_NO_ERROR ) {
                        sprintf( ftpunoBuffer, "Error code= %d", diff );
                        printToStatusBox( "Error obtaining file info.", ftpunoBuffer );
                    }
                    else {
                        if ( fileOrDirectory1 == '>' ) {
                            // Navigate down through directory
                            if ( *ftpunoBuffer == '.' && *( ftpunoBuffer + 1 ) == 0 ) {
                                // '.' is the same directory, just do nothing
                                diff = true;
                            }
                            else if ( *ftpunoBuffer == '.' && *( ftpunoBuffer + 1 ) == '.' && *( ftpunoBuffer + 2 ) == 0 ) {
                                // ".." is up one directory
                                pathUpOneDir( ftpunoFTPPath );
                                diff = true;
                            }
                            else {
                                // Regular directory
                                diff = concatPath( ftpunoFTPPath, ftpunoBuffer, FTP_PATH_SIZE );
                                if ( diff == false ) {
                                    printToStatusBox( "File path too long.", NULL );
                                }
                            }
                            if ( diff == true ) {
                                brightSelection( false );
                                firstDisplayedFTPEntry = 0;
                                refreshFTPListing();
                                selectedDisplayedEntry = 0;
                                brightSelection( true );
                                showFileInfo = true;
                            }
                        }
                        else {
                            // Download the file with confirmation

                            // Obtain full FTP file path in ftpunoFilePath
                            strcpy( ftpunoFilePath, ftpunoFTPPath );
                            pbuffer = ftpunoFilePath;
                            while ( *pbuffer > 0 ) {
                                pbuffer++;
                            }
                            pbuffer2 = ftpunoBuffer;
                            while ( *pbuffer2 > 0 ) {
                                *pbuffer++ = *pbuffer2++;
                            }
                            *pbuffer++ = 0;

                            // Copy sd path
                            strcpy( pbuffer, ftpunoSDPath );
                            pbuffer2 = pbuffer;
                            while ( *pbuffer2 > 0 ) {
                                pbuffer2++;
                            }
                            //Copy sd filename, shortened to 8.3 name
                            pbuffer3 = ftpunoBuffer;
                            j = 0;
                            for( i = 0; i < 8 && *pbuffer3 > 0; i++ ) {
                                if ( *pbuffer3 == '.' ) {
                                    j = 1;
                                    *pbuffer2 = '.';
                                }
                                else if ( *pbuffer3 == ' ' ) {
                                    *pbuffer2 = '_';
                                }
                                else {
                                    *pbuffer2 = *pbuffer3;
                                }
                                pbuffer2++;
                                pbuffer3++;
                            }
                            if ( *pbuffer3 > 0 ) {
                                i = strlen( pbuffer3 );
                                k = 3;
                                if ( i < 3 ) {
                                    k = i;
                                }
                                pbuffer3 += i - k;
                                if ( j == 0 && *( pbuffer3 - 1 ) == '.' ) {
                                    *pbuffer2++ = '.';
                                }
                                while ( k > 0 ) {
                                    *pbuffer2++ = *pbuffer3++;
                                    k--;
                                }
                            }
                            *pbuffer2 = 0;

                            sprintf( ftpunoBuffer, "Confirm DOWNLOAD %ld bytes?", fileSize1 );
                            printToStatusBox( ftpunoBuffer, shortString( pbuffer, 24 ) );
                            textUtils_printAt32( 27, 23 );
                            textUtils_print( "(Y/N)" );

                            key2 = waitKey();

                            if ( key2 == 'y' || key2 == 'Y' ) {
                                
                                printToStatusBox( "Downloading...", NULL );

                                diff = FTP_downloadFile( ftpunoFilePath, pbuffer, ftpunoBuffer, BUFFER_SIZE, downloadProgressCallback );
                                if ( diff == FTP_NO_ERROR ) {

                                    printToStatusBox( "File downloaded succesfully:", shortString( pbuffer, X_SIZE_STATUS_BOX ) );

                                    refreshSDListing();
                                    showFileInfo = false;

                                }
                                else {

                                    sprintf( ftpunoBuffer, "Err code: %d", diff );
                                    printToStatusBox( "Error while downloading.", ftpunoBuffer );

                                }
                            }
                            else {
                                printToStatusBox( "Aborted by user.", NULL );
                            }
                        }
                    }
                }
                else {
                    if ( getSDFileNameAndSize( ftpunoFilePath, &fileSize1, &fileOrDirectory1 ) == true ) {
                        if ( fileOrDirectory1 == ESXDOS_FILE_ATTRIBUTE_DIR_BIT ) {
                            // Navigate down through directory
                            if ( *ftpunoFilePath == '.' && *( ftpunoFilePath + 1 ) == 0 ) {
                                // '.' is the same directory, just do nothing
                                diff = true;
                            }
                            else if ( *ftpunoFilePath == '.' && *( ftpunoFilePath + 1 ) == '.' && *( ftpunoFilePath + 2 ) == 0 ) {
                                // ".." is up one directory
                                pathUpOneDir( ftpunoSDPath );
                                diff = true;
                            }
                            else {
                                // Regular directory
                                diff = concatPath( ftpunoSDPath, ftpunoFilePath, FTP_PATH_SIZE );
                                if ( diff == false ) {
                                    printToStatusBox( "File path too long.", NULL );
                                }
                            }
                            if ( diff == true ) {
                                brightSelection( false );
                                firstDisplayedSDEntry = 0;
                                refreshSDListing();
                                selectedDisplayedEntry = 0;
                                brightSelection( true );
                                showFileInfo = true;
                            }
                        }
                        else {
                            // Delete the file with confirmation
                            sprintf( ftpunoBuffer, "Confirm DELETE %ld bytes?", fileSize1 );
                            printToStatusBox( ftpunoBuffer, shortString( ftpunoFilePath, 24 ) );
                            textUtils_printAt32( 27, 23 );
                            textUtils_print( "(Y/N)" );
                            
                            key2 = waitKey();

                            if ( key2 == 'y' || key2 == 'Y' ) {
                                
                                printToStatusBox( "Deleting file...", NULL );

                                ESXDOS_delete( ftpunoFilePath, SD_drive );
                                iferror {

                                    sprintf( ftpunoBuffer, "Err code: %d", diff );
                                    printToStatusBox( "Error while deleting file:", shortString( ftpunoFilePath, X_SIZE_STATUS_BOX ) );

                                }
                                else {

                                    printToStatusBox( "File deleted succesfully:", shortString( ftpunoFilePath, X_SIZE_STATUS_BOX ) );

                                    if ( selectedDisplayedEntry == 0 ) {
                                        if ( firstDisplayedSDEntry > 0 && ( firstDisplayedSDEntry + 1 == numTotalSDEntries ) ) {
                                            
                                            // User has deleted the only one file in the page. Go to previous page.

                                            diff = MAX_DISPLAY_DIR_ENTRIES;
                                            if ( diff > firstDisplayedSDEntry ) {
                                                diff = firstDisplayedSDEntry;
                                            }
                                            firstDisplayedSDEntry -= diff;

                                            brightSelection( false );

                                            refreshSDListing();

                                            selectedDisplayedEntry = numDisplayedSDEntries - 1;

                                            brightSelection( true );
                                        }
                                    }
                                    else {
                                        if ( selectedDisplayedEntry + 1 >= numDisplayedSDEntries ) {
                                            brightSelection( false );
                                            selectedDisplayedEntry--;
                                            refreshSDListing();
                                            brightSelection( true );
                                        }
                                        else {
                                            refreshSDListing();
                                            brightSelection( true );
                                        }
                                    }
                                }
                                
                                showFileInfo = false;
                            }
                            else {
                                printToStatusBox( "Aborted by user.", NULL );
                            }
                        }
                    }
                    else {
                        printToStatusBox( "Error obtaining SD file info.", NULL );
                    }
                }
                break;

            // Backspace pressed
            case 12:
                brightSelection( false );
                firstDisplayedFTPEntry = 0;
                if ( selectedFTP_NO_SD == true ) {
                    pathUpOneDir( ftpunoFTPPath );
                    refreshFTPListing();
                }
                else {
                    pathUpOneDir( ftpunoSDPath );
                    refreshSDListing();
                }
                selectedDisplayedEntry = 0;
                brightSelection( true );
                showFileInfo = true;
        }

        if ( showFileInfo == true ) {
            if ( selectedFTP_NO_SD == true ) {
                showFTPFileInfo();
            }
            else {
                showSDFileInfo();
            }
        }

    }

    i = waitKey();
    while ( i != 32 ) {
        i = waitKey();
        textUtils_println_l( i );
    }
}
