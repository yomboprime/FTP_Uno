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

Ported to ZX-Uno by yomboprime

--------------------------------------------------------------------*/

#ifndef EspDrv_h
#define EspDrv_h

#include "../integerTypes.h"
#include "IPAddress.h"
#include "RingBuffer.h"


// Maximum size of a SSID
#define WL_SSID_MAX_LENGTH 32

// Size of a MAC-address or BSSID
#define WL_MAC_ADDR_LENGTH 6

// Size of a MAC-address or BSSID
#define WL_IPV4_LENGTH 4

// Maximum size of a SSID list
#define WL_NETWORKS_LIST_MAXNUM 10

// Maximum number of socket
#define MAX_SOCK_NUM        4

// Socket not available constant
#define SOCK_NOT_AVAIL  255

// Default state value for Wifi state field
#define NA_STATE -1

#define WL_FW_VER_LENGTH 6

#define NO_SOCKET_AVAIL 255


// maximum size of AT command
#define CMD_BUFFER_SIZE 200


typedef enum eProtMode {TCP_MODE, UDP_MODE, SSL_MODE} tProtMode;

/*
orig
typedef enum {
        WL_FAILURE = -1,
        WL_SUCCESS = 1,
} wl_error_code_t;
*/

/* Authentication modes */
enum wl_auth_mode {
        AUTH_MODE_INVALID,
        AUTH_MODE_AUTO,
        AUTH_MODE_OPEN_SYSTEM,
        AUTH_MODE_SHARED_KEY,
        AUTH_MODE_WPA,
        AUTH_MODE_WPA2,
        AUTH_MODE_WPA_PSK,
        AUTH_MODE_WPA2_PSK
};


typedef enum {
    WL_NO_SHIELD = 255,
    WL_IDLE_STATUS = 0,
    //WL_NO_SSID_AVAIL,
    //WL_SCAN_COMPLETED,
    WL_CONNECTED,
    WL_CONNECT_FAILED,
    //WL_CONNECTION_LOST,
    WL_DISCONNECTED
} wl_status_t;

/* Encryption modes */
enum wl_enc_type {
    ENC_TYPE_NONE = 0,
    ENC_TYPE_WEP = 1,
    ENC_TYPE_WPA_PSK = 2,
    ENC_TYPE_WPA2_PSK = 3,
    ENC_TYPE_WPA_WPA2_PSK = 4
};


enum wl_tcp_state {
    CLOSED      = 0,
    LISTEN      = 1,
    SYN_SENT    = 2,
    SYN_RCVD    = 3,
    ESTABLISHED = 4,
    FIN_WAIT_1  = 5,
    FIN_WAIT_2  = 6,
    CLOSE_WAIT  = 7,
    CLOSING     = 8,
    LAST_ACK    = 9,
    TIME_WAIT   = 10
};





extern bool EspDrv_wifiDriverInit();

extern void EspDrv_reset();

/* Start Wifi connection with passphrase
 *
 * param ssid: Pointer to the SSID string.
 * param passphrase: Passphrase. Valid characters in a passphrase must be between ASCII 32-126 (decimal).
 */
extern bool EspDrv_wifiConnect(char* ssid, char *passphrase);


/*
* Start the Access Point
*/
extern bool EspDrv_wifiStartAP(char* ssid, char* pwd, uint8_t channel, uint8_t enc, uint8_t espMode);


/*
 * Set ip configuration disabling dhcp client
 */
extern void EspDrv_config(IPAddress* ip);

/*
 * Set ip configuration disabling dhcp client
 */
extern void EspDrv_configAP(IPAddress* ip);


/*
 * Disconnect from the network
 *
 * return: WL_SUCCESS or WL_FAILURE
 */
extern int8_t EspDrv_disconnect();

/*
 *
 *
 * return: one value of wl_status_t enum
 */
extern uint8_t EspDrv_getConnectionStatus();

/*
 * Get the interface MAC address.
 *
 * return: pointer to uint8_t array with length WL_MAC_ADDR_LENGTH
 */
extern uint8_t* EspDrv_getMacAddress();

/*
 * Get the interface IP address.
 *
 * return: copy the ip address value in IPAddress object
 */
extern void EspDrv_getIpAddress(IPAddress* ip);

extern void EspDrv_getIpAddressAP(IPAddress* ip);

/*
 * Get the interface IP netmask.
 * This can be used to retrieve settings configured through DHCP.
 *
 * return: true if successful
 */
extern bool EspDrv_getNetmask(IPAddress* mask);

/*
 * Get the interface IP gateway.
 * This can be used to retrieve settings configured through DHCP.
 *
 * return: true if successful
 */
extern bool EspDrv_getGateway(IPAddress* mask);

/*
 * Return the current SSID associated with the network
 *
 * return: ssid string
 */
extern char* EspDrv_getCurrentSSID();

/*
 * Return the current BSSID associated with the network.
 * It is the MAC address of the Access Point
 *
 * return: pointer to uint8_t array with length WL_MAC_ADDR_LENGTH
 */
extern uint8_t* EspDrv_getCurrentBSSID();

/*
 * Return the current RSSI /Received Signal Strength in dBm)
 * associated with the network
 *
 * return: signed value
 */
extern int32_t EspDrv_getCurrentRSSI();

/*
 * Get the networks available
 *
 * return: Number of discovered networks
 */
extern uint8_t EspDrv_getScanNetworks();

/*
 * Return the SSID discovered during the network scan.
 *
 * param networkItem: specify from which network item want to get the information
 *
 * return: ssid string of the specified item on the networks scanned list
 */
extern char* EspDrv_getSSIDNetworks(uint8_t networkItem);

/*
 * Return the RSSI of the networks discovered during the scanNetworks
 *
 * param networkItem: specify from which network item want to get the information
 *
 * return: signed value of RSSI of the specified item on the networks scanned list
 */
extern int32_t EspDrv_getRSSINetworks(uint8_t networkItem);

/*
 * Return the encryption type of the networks discovered during the scanNetworks
 *
 * param networkItem: specify from which network item want to get the information
 *
 * return: encryption type (enum wl_enc_type) of the specified item on the networks scanned list
 */
extern uint8_t EspDrv_getEncTypeNetworks(uint8_t networkItem);


/*
 * Get the firmware version
 */
extern char* EspDrv_getFwVersion();


////////////////////////////////////////////////////////////////////////////
// Client/Server methods
////////////////////////////////////////////////////////////////////////////


extern bool EspDrv_startServer(uint16_t port);
extern bool EspDrv_startClient(char* host, uint16_t port, uint8_t sock, uint8_t protMode);
extern bool EspDrv_stopClient(uint8_t sock);
extern bool EspDrv_getServerState(uint8_t sock);
extern bool EspDrv_getClientState(uint8_t sock);
extern bool EspDrv_getData(uint8_t socket, uint8_t *data, bool peek, bool* connClose);
extern int EspDrv_getDataBuf(uint8_t socket, uint8_t *buf, uint16_t bufSize, bool* connClose);
extern bool EspDrv_sendData(uint8_t socket, uint8_t *data, uint16_t len, bool appendCrLf);
// delete: bool EspDrv_sendData(uint8_t sock, const __FlashStringHelper *data, uint16_t len, bool appendCrLf=false);
extern bool EspDrv_sendDataUdp(uint8_t sock, char* host, uint16_t port, uint8_t *data, uint16_t len);
extern uint16_t EspDrv_availData(uint8_t socket);
extern uint8_t EspDrv_getConnId();

extern bool EspDrv_ping(char *host);

extern void EspDrv_getRemoteIpAddress(IPAddress* ip);
extern uint16_t EspDrv_getRemotePort();


////////////////////////////////////////////////////////////////////////////////

extern int EspDrv_sendCmd(char* cmd, int timeout);
// orig static int sendCmd(const __FlashStringHelper* cmd, int timeout=1000);
// orig static int sendCmd(const __FlashStringHelper* cmd, int timeout, ...);

extern int EspDrv_sendCmdBuffer( int timeout );

extern bool EspDrv_sendCmdGet(char* cmd, char* startTag, char* endTag, char* outStr, int outStrLen);

extern int EspDrv_readUntil(int timeout, char* tag, bool findTags);

extern void EspDrv_espEmptyBuf(bool warn);

extern int EspDrv_timedRead();

#endif // EspDrv_h
