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

#include "EspDrv.h"

#include <string.h>
#include <stdlib.h>

#include "debugEsp.h"
#include "../zxuno/zxuno.h"
#include "../zxuno/uart.h"

#define NUMESPTAGS 5

char* ESPTAGS[] =
{
    "\n\rOK\n\r",
    "\n\rERROR\n\r",
    "\n\rFAIL\n\r",
    "\n\rSEND OK\n\r",
    " CONNECT\n\r"
};

typedef enum
{
    TAG_OK,
    TAG_ERROR,
    TAG_FAIL,
    TAG_SENDOK,
    TAG_CONNECT
} TagsEnum;


// Array of data to cache the information related to the networks discovered
char    EspDrv__networkSsid[WL_NETWORKS_LIST_MAXNUM * WL_SSID_MAX_LENGTH];
int32_t EspDrv__networkRssi[WL_NETWORKS_LIST_MAXNUM] = { 0 };
uint8_t EspDrv__networkEncr[WL_NETWORKS_LIST_MAXNUM] = { 0 };

// Cached values of retrieved data
// settings of current selected network
char     EspDrv__ssid[WL_SSID_MAX_LENGTH];
uint8_t  EspDrv__bssid[WL_MAC_ADDR_LENGTH];
uint8_t  EspDrv__mac[WL_MAC_ADDR_LENGTH];
uint8_t  EspDrv__localIp[WL_IPV4_LENGTH];

char EspDrv_fwVersion[WL_FW_VER_LENGTH];

long EspDrv__bufPos=0;
uint8_t EspDrv__connId=0;

uint16_t EspDrv__remotePort = 0;
uint8_t EspDrv__remoteIp[WL_IPV4_LENGTH];

uint8_t EspDrv_commandBuffer[ CMD_BUFFER_SIZE ];

bool EspDrv_wifiDriverInit()
{
    uint16_t i;
    uint8_t *pc;

    bool initOK;

    EspDrv_fwVersion[ 0 ] = 0;

    EspDrv__remoteIp[ 0 ] = 0;
    EspDrv__remoteIp[ 1 ] = 0;
    EspDrv__remoteIp[ 2 ] = 0;
    EspDrv__remoteIp[ 3 ] = 0;

    memset( EspDrv__networkSsid, 0, WL_NETWORKS_LIST_MAXNUM * WL_SSID_MAX_LENGTH );

    pc = EspDrv__networkSsid;
    for ( i = 0; i < WL_NETWORKS_LIST_MAXNUM; i++ ) {

        sprintf( pc, "%u", i );

        pc += WL_SSID_MAX_LENGTH;

    }

    LOGDEBUG(F("> wifiDriverInit"));

    RingBuffer_create();

    UART_begin();

    initOK = false;

    for(i=0; i<1; i++) // kkk was 5 tries
    {
        if (EspDrv_sendCmd("AT", 10000) == TAG_OK)
        {
            initOK=true;
            break;
        }
        delay(1000);
    }

    if (!initOK)
    {
        LOGERROR(F("Cannot initialize ESP module"));
        delay(1000);
        return false;
    }

    EspDrv_reset();

    // check firmware version
    EspDrv_getFwVersion();

    // prints a warning message if the firmware is not 1.X
    if (EspDrv_fwVersion[0] != '1' ||
        EspDrv_fwVersion[1] != '.')
    {
        LOGWARN1(F("Warning: Unsupported firmware"), EspDrv_fwVersion);
        delay(4000);
    }
    else
    {
        LOGINFO1(F("Initilization successful -"), EspDrv_fwVersion);
    }

    return true;
}


void EspDrv_reset()
{
    LOGDEBUG(F("> reset"));

    EspDrv_sendCmd("AT+RST", 1000);
    delay(3000);
    EspDrv_espEmptyBuf(false);  // empty dirty characters from the buffer

    // disable echo of commands
    EspDrv_sendCmd("ATE0", 1000);

    // set station mode
    EspDrv_sendCmd("AT+CWMODE=1", 1000);
    delay(200);

    // set multiple connections mode
    EspDrv_sendCmd("AT+CIPMUX=1", 1000);

    // Show remote IP and port with "+IPD"
    EspDrv_sendCmd("AT+CIPDINFO=1", 1000);

    // Disable autoconnect
    // Automatic connection can create problems during initialization phase at next boot
    EspDrv_sendCmd("AT+CWAUTOCONN=0", 1000);

    // enable DHCP
    EspDrv_sendCmd("AT+CWDHCP=1,1", 1000);
    delay(200);
}



bool EspDrv_wifiConnect(char* ssid, char *passphrase)
{
    int ret;

    LOGDEBUG(F("> wifiConnect"));

    // TODO
    // Escape character syntax is needed if "SSID" or "password" contains
    // any special characters (',', '"' and '/')

    // connect to access point, use CUR mode to avoid connection at boot
    //orig int ret = sendCmd("AT+CWJAP_CUR=\"%s\",\"%s\"", 20000, ssid, passphrase);
    sprintf( EspDrv_commandBuffer, "AT+CWJAP_CUR=\"%s\",\"%s\"", ssid, passphrase );
    ret = EspDrv_sendCmdBuffer( 10000 );

    if (ret==TAG_OK)
    {
        LOGINFO1(F("Connected to"), ssid);
        return true;
    }

    LOGWARN1(F("Failed connecting to"), ssid);

    // clean additional messages logged after the FAIL tag
    delay(1000);
    EspDrv_espEmptyBuf(false);

    return false;
}


bool EspDrv_wifiStartAP(char* ssid, char* pwd, uint8_t channel, uint8_t enc, uint8_t espMode)
{
    int ret;

    LOGDEBUG(F("> wifiStartAP"));

    // set AP mode, use CUR mode to avoid automatic start at boot
    //orig int ret = sendCmd(F("AT+CWMODE_CUR=%d"), 10000, espMode);
    sprintf( EspDrv_commandBuffer, "AT+CWMODE_CUR=%d", espMode );
    ret = EspDrv_sendCmdBuffer( 10000 );

    if (ret!=TAG_OK)
    {
        LOGWARN1(F("Failed to set AP mode"), ssid);
        return false;
    }

    // TODO
    // Escape character syntax is needed if "SSID" or "password" contains
    // any special characters (',', '"' and '/')

    // start access point
    //orig ret = sendCmd(F("AT+CWSAP_CUR=\"%s\",\"%s\",%d,%d"), 10000, ssid, pwd, channel, enc);
    sprintf( EspDrv_commandBuffer, "AT+CWSAP_CUR=\"%s\",\"%s\",%d,%d", ssid, pwd, channel, enc);
    ret = EspDrv_sendCmdBuffer( 10000 );

    if (ret!=TAG_OK)
    {
        LOGWARN1(F("Failed to start AP"), ssid);
        return false;
    }

    if (espMode==2)
        EspDrv_sendCmd("AT+CWDHCP_CUR=0,1", 1000);    // enable DHCP for AP mode
    if (espMode==3)
        EspDrv_sendCmd("AT+CWDHCP_CUR=2,1", 1000);    // enable DHCP for station and AP mode

    LOGINFO1(F("Access point started"), ssid);
    return true;
}


int8_t EspDrv_disconnect()
{
    LOGDEBUG(F("> disconnect"));

    if(EspDrv_sendCmd("AT+CWQAP", 1000)==TAG_OK)
        return WL_DISCONNECTED;

    // wait and clear any additional message
    delay(2000);
    EspDrv_espEmptyBuf(false);

    return WL_DISCONNECTED;
}

void EspDrv_config(IPAddress* ip)
{
    int ret;
    char buf[16];

    LOGDEBUG(F("> config"));

    // disable station DHCP
    EspDrv_sendCmd("AT+CWDHCP_CUR=1,0", 1000);

    // it seems we need to wait here...
    delay(500);

    // TODO use special format flag specific for ipv4
    sprintf(buf, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

    //orig int ret = sendCmd(F("AT+CIPSTA_CUR=\"%s\""), 2000, buf);
    sprintf( EspDrv_commandBuffer, "AT+CIPSTA_CUR=\"%s\"", buf);
    ret = EspDrv_sendCmdBuffer( 2000 );

    delay(500);

    if (ret==TAG_OK)
    {
        LOGINFO1(F("IP address set"), buf);
    }
}

void EspDrv_configAP(IPAddress* ip)
{
    int ret;
    char buf[16];

    LOGDEBUG(F("> config"));

    EspDrv_sendCmd("AT+CWMODE_CUR=2", 1000);

    // disable station DHCP
    EspDrv_sendCmd("AT+CWDHCP_CUR=2,0", 1000);

    // it seems we need to wait here...
    delay(500);

    // TODO use special format flag specific for ipv4
    sprintf(buf, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

    //orig ret = sendCmd(F("AT+CIPAP_CUR=\"%s\""), 2000, buf);
    sprintf( EspDrv_commandBuffer, "AT+CIPAP_CUR=\"%s\"", buf);
    ret = EspDrv_sendCmdBuffer( 2000 );
    delay(500);

    if (ret==TAG_OK)
    {
        LOGINFO1(F("IP address set"), buf);
    }
}

uint8_t EspDrv_getConnectionStatus()
{
    char buf[10];
    int s;

    LOGDEBUG(F("> getConnectionStatus"));

/*
    AT+CIPSTATUS

    Response

        STATUS:<stat>
        +CIPSTATUS:<link ID>,<type>,<remote_IP>,<remote_port>,<local_port>,<tetype>

    Parameters

        <stat>
            2: Got IP
            3: Connected
            4: Disconnected
        <link ID> ID of the connection (0~4), for multi-connect
        <type> string, "TCP" or "UDP"
        <remote_IP> string, remote IP address.
        <remote_port> remote port number
        <local_port> ESP8266 local port number
        <tetype>
            0: ESP8266 runs as client
            1: ESP8266 runs as server
*/

    if(!EspDrv_sendCmdGet(F("AT+CIPSTATUS"), F("STATUS:"), F("\n\r"), buf, sizeof(buf)))
        return WL_NO_SHIELD;

    // 4: client disconnected
    // 5: wifi disconnected
    s = atoi(buf);
    if(s==2 || s==3 || s==4)
        return WL_CONNECTED;
    else if(s==5)
        return WL_DISCONNECTED;

    return WL_IDLE_STATUS;
}

bool EspDrv_getClientState(uint8_t socket)
{
    char findBuf[20];
    char buf[10];

    LOGDEBUG1(F("> getClientState"), socket);

    sprintf(findBuf, "+CIPSTATUS:%d,", socket);

    if (EspDrv_sendCmdGet(F("AT+CIPSTATUS"), findBuf, ",", buf, sizeof(buf)))
    {
        LOGDEBUG(F("Connected"));
        return true;
    }

    LOGDEBUG(F("Not connected"));
    return false;
}

uint8_t* EspDrv_getMacAddress()
{
    char buf[20];
    char* token;

    LOGDEBUG(F("> getMacAddress"));

    memset(EspDrv__mac, 0, WL_MAC_ADDR_LENGTH);

    if (EspDrv_sendCmdGet(F("AT+CIFSR"), F(":STAMAC,\""), F("\""), buf, sizeof(buf)))
    {

        token = strtok(buf, ":");
        EspDrv__mac[5] = (uint8_t)strtol(token, NULL, 16);
        token = strtok(NULL, ":");
        EspDrv__mac[4] = (uint8_t)strtol(token, NULL, 16);
        token = strtok(NULL, ":");
        EspDrv__mac[3] = (uint8_t)strtol(token, NULL, 16);
        token = strtok(NULL, ":");
        EspDrv__mac[2] = (uint8_t)strtol(token, NULL, 16);
        token = strtok(NULL, ":");
        EspDrv__mac[1] = (uint8_t)strtol(token, NULL, 16);
        token = strtok(NULL, ":");
        EspDrv__mac[0] = (uint8_t)strtol(token, NULL, 16);
    }
    return EspDrv__mac;
}


void EspDrv_getIpAddress(IPAddress* ip)
{
    char buf[20];
    char* token;

    LOGDEBUG(F("> getIpAddress"));

    if (EspDrv_sendCmdGet(F("AT+CIFSR"), F(":STAIP,\""), F("\""), buf, sizeof(buf)))
    {
        // TODO use IPAddr_parse to eliminate the need of strtok and reduce memory usage
        token = strtok(buf, ".");
        EspDrv__localIp[0] = atoi(token);
        token = strtok(NULL, ".");
        EspDrv__localIp[1] = atoi(token);
        token = strtok(NULL, ".");
        EspDrv__localIp[2] = atoi(token);
        token = strtok(NULL, ".");
        EspDrv__localIp[3] = atoi(token);

        //orig ip = _localIp;
        IPAddress_copy( EspDrv__localIp, ip );
    }
}

void EspDrv_getIpAddressAP(IPAddress* ip)
{
    char buf[20];
    char* token;

    LOGDEBUG(F("> getIpAddressAP"));

    if (EspDrv_sendCmdGet(F("AT+CIPAP?"), F("+CIPAP:ip:\""), F("\""), buf, sizeof(buf)))
    {
        token = strtok(buf, ".");
        EspDrv__localIp[0] = atoi(token);
        token = strtok(NULL, ".");
        EspDrv__localIp[1] = atoi(token);
        token = strtok(NULL, ".");
        EspDrv__localIp[2] = atoi(token);
        token = strtok(NULL, ".");
        EspDrv__localIp[3] = atoi(token);

        //orig ip = _localIp;
        IPAddress_copy( EspDrv__localIp, ip );
    }
}



char* EspDrv_getCurrentSSID()
{
    LOGDEBUG(F("> getCurrentSSID"));

    EspDrv__ssid[0] = 0;
    EspDrv_sendCmdGet(F("AT+CWJAP?"), F("+CWJAP:\""), F("\""), EspDrv__ssid, sizeof(EspDrv__ssid));

    return EspDrv__ssid;
}

uint8_t* EspDrv_getCurrentBSSID()
{
    char buf[20];
    char* token;

    LOGDEBUG(F("> getCurrentBSSID"));

    memset(EspDrv__bssid, 0, WL_MAC_ADDR_LENGTH);

    if (EspDrv_sendCmdGet(F("AT+CWJAP?"), F(",\""), F("\","), buf, sizeof(buf)))
    {
        token = strtok(buf, ":");
        EspDrv__bssid[5] = (uint8_t)strtol(token, NULL, 16);
        token = strtok(NULL, ":");
        EspDrv__bssid[4] = (uint8_t)strtol(token, NULL, 16);
        token = strtok(NULL, ":");
        EspDrv__bssid[3] = (uint8_t)strtol(token, NULL, 16);
        token = strtok(NULL, ":");
        EspDrv__bssid[2] = (uint8_t)strtol(token, NULL, 16);
        token = strtok(NULL, ":");
        EspDrv__bssid[1] = (uint8_t)strtol(token, NULL, 16);
        token = strtok(NULL, ":");
        EspDrv__bssid[0] = (uint8_t)strtol(token, NULL, 16);
    }
    return EspDrv__bssid;

}

int32_t EspDrv_getCurrentRSSI()
{
    int ret;
    char buf[10];

    LOGDEBUG(F("> getCurrentRSSI"));

    ret=0;
    EspDrv_sendCmdGet(F("AT+CWJAP?"), F(",-"), F("\n\r"), buf, sizeof(buf));

    if (isDigit(buf[0])) {
      ret = -atoi(buf);
    }

    return ret;
}


uint8_t EspDrv_getScanNetworks()
{
    uint8_t ssidListNum = 0;
    int idx;
    bool ret = false;
    char buf[100];
    char *ssid;

    EspDrv_espEmptyBuf( true );

    LOGDEBUG(F("----------------------------------------------"));
    LOGDEBUG(F(">> AT+CWLAP"));

    //orig espSerial->println(F("AT+CWLAP"));
    UART_println( "AT+CWLAP" );

    idx = EspDrv_readUntil(10000, "+CWLAP:(", true);

    while (idx == NUMESPTAGS)
    {
        EspDrv__networkEncr[ssidListNum] = UART_parseInt(1000);

        // discard , and " characters
        EspDrv_readUntil(1000, "\"", true);

        idx = EspDrv_readUntil(1000, "\"", false);
        if(idx==NUMESPTAGS)
        {
            ssid = &EspDrv__networkSsid[ ssidListNum * WL_SSID_MAX_LENGTH ];
            memset( ssid, 0, WL_SSID_MAX_LENGTH );
            RingBuffer_getStrN(ssid, 1, WL_SSID_MAX_LENGTH-1);
        }

        // discard , character
        EspDrv_readUntil(1000, ",", true);

        EspDrv__networkRssi[ssidListNum] = UART_parseInt(1000);

        idx = EspDrv_readUntil(1000, "+CWLAP:(", true);

        if(ssidListNum==WL_NETWORKS_LIST_MAXNUM-1)
            break;

        ssidListNum++;
    }

    if (idx==-1)
        return -1;

    LOGDEBUG1(F("---------------------------------------------- >"), ssidListNum);
    return ssidListNum;
}

bool EspDrv_getNetmask(IPAddress* mask) {

    char buf[20];

    LOGDEBUG(F("> getNetmask"));

    if (EspDrv_sendCmdGet(F("AT+CIPSTA?"), F("+CIPSTA:netmask:\""), F("\""), buf, sizeof(buf)))
    {
        //orig mask.fromString (buf);
        IPAddress_parse( buf, mask );
        return true;
    }

    return false;
}

bool EspDrv_getGateway(IPAddress* gw)
{
    char buf[20];

    LOGDEBUG(F("> getGateway"));

    if (EspDrv_sendCmdGet(F("AT+CIPSTA?"), F("+CIPSTA:gateway:\""), F("\""), buf, sizeof(buf)))
    {
        //orig gw.fromString (buf);
        IPAddress_parse( buf, gw );
        return true;
    }

    return false;
}

char* EspDrv_getSSIDNetworks(uint8_t networkItem)
{
    if (networkItem >= WL_NETWORKS_LIST_MAXNUM)
        return NULL;

    return &EspDrv__networkSsid[ networkItem * WL_SSID_MAX_LENGTH ];
}

uint8_t EspDrv_getEncTypeNetworks(uint8_t networkItem)
{
    if (networkItem >= WL_NETWORKS_LIST_MAXNUM)
        return NULL;

    return EspDrv__networkEncr[networkItem];
}

int32_t EspDrv_getRSSINetworks(uint8_t networkItem)
{
    if (networkItem >= WL_NETWORKS_LIST_MAXNUM)
        return NULL;

    return EspDrv__networkRssi[networkItem];
}

char* EspDrv_getFwVersion()
{
    LOGDEBUG(F("> getFwVersion"));

    EspDrv_fwVersion[0] = 0;

    EspDrv_sendCmdGet(F("AT+GMR"), F("SDK version:"), F("\n\r"), EspDrv_fwVersion, sizeof(EspDrv_fwVersion));

    return EspDrv_fwVersion;
}



bool EspDrv_ping(char *host)
{
    int ret;

    LOGDEBUG(F("> ping"));

    //orig ret = sendCmd(F("AT+PING=\"%s\""), 8000, host);
    sprintf( EspDrv_commandBuffer, "AT+PING=\"%s\"", host);
    ret = EspDrv_sendCmdBuffer( 8000 );

    if (ret==TAG_OK)
        return true;

    return false;
}



// Start server TCP on port specified
bool EspDrv_startServer(uint16_t port)
{
    int ret;

    LOGDEBUG1(F("> startServer"), port);

    //orig ret = sendCmd(F("AT+CIPSERVER=1,%d"), 1000, port);
    sprintf( EspDrv_commandBuffer, "AT+CIPSERVER=1,%d", port);
    ret = EspDrv_sendCmdBuffer( 1000 );

    return ret==TAG_OK;
}


bool EspDrv_startClient(char* host, uint16_t port, uint8_t socket, uint8_t protMode)
{
    int ret;

    LOGDEBUG2ssi(F("> startClient"), host, port);

    // TCP
    // AT+CIPSTART=<link ID>,"TCP",<remote IP>,<remote port>

    // UDP
    // AT+CIPSTART=<link ID>,"UDP",<remote IP>,<remote port>[,<UDP local port>,<UDP mode>]

    // for UDP we set a dummy remote port and UDP mode to 2
    // this allows to specify the target host/port in CIPSEND

    if ( protMode==TCP_MODE ) {
        //orig ret = sendCmd(F("AT+CIPSTART=%d,\"TCP\",\"%s\",%u"), 5000, sock, host, port);
        sprintf( EspDrv_commandBuffer, "AT+CIPSTART=%d,\"TCP\",\"%s\",%u", socket, host, port);
    }
    else if (protMode==SSL_MODE) {
        // better to put the CIPSSLSIZE here because it is not supported before firmware 1.4
        EspDrv_sendCmd("AT+CIPSSLSIZE=4096", 1000);
        //orig ret = sendCmd(F("AT+CIPSTART=%d,\"SSL\",\"%s\",%u"), 5000, sock, host, port);
        sprintf( EspDrv_commandBuffer, "AT+CIPSTART=%d,\"TCP\",\"%s\",%u", socket, host, port);
    }
    else if (protMode==UDP_MODE) {
        //orig ret = sendCmd(F("AT+CIPSTART=%d,\"UDP\",\"%s\",0,%u,2"), 5000, sock, host, port);
        sprintf( EspDrv_commandBuffer, "AT+CIPSTART=%d,\"UDP\",\"%s\",0,%u,2", socket, host, port);
    }
    ret = EspDrv_sendCmdBuffer( 5000 );

    return ret==TAG_OK;
}


// Start server TCP on port specified
bool EspDrv_stopClient(uint8_t socket)
{
    int ret;

    LOGDEBUG1(F("> stopClient"), socket);

    //orig ret = sendCmd(F("AT+CIPCLOSE=%d"), 4000, sock);
    sprintf( EspDrv_commandBuffer, "AT+CIPCLOSE=%d", socket);
    ret = EspDrv_sendCmdBuffer( 4000 );

    return ret == TAG_OK;
    
}


bool EspDrv_getServerState(uint8_t socket)
{
    return false;
}



////////////////////////////////////////////////////////////////////////////
// TCP/IP functions
////////////////////////////////////////////////////////////////////////////


uint16_t EspDrv_availData(uint8_t socket)
{
    int bytes;

    //LOGDEBUG(bufPos);

    // if there is data in the buffer
    if (EspDrv__bufPos>0)
    {
        if (EspDrv__connId==socket)
            return EspDrv__bufPos;
        else if (EspDrv__connId==0)
            return EspDrv__bufPos;
    }


    bytes = UART_available();

    if ( bytes > 0 )
    {
        //LOGDEBUG1(F("Bytes in the serial buffer: "), bytes);
        if ( UART_find((char *)"+IPD,", 1000) == true )
        {
            // format is : +IPD,<id>,<len>:<data>
            // format is : +IPD,<ID>,<len>[,<remote IP>,<remote port>]:<data>

            EspDrv__connId = UART_parseInt(1000);    // <ID>
            UART_read();                         // ,
            EspDrv__bufPos = UART_parseInt(1000);    // <len>
            UART_read();                         // "
            EspDrv__remoteIp[0] = UART_parseInt(1000);    // <remote IP>
            UART_read();                         // .
            EspDrv__remoteIp[1] = UART_parseInt(1000);
            UART_read();                         // .
            EspDrv__remoteIp[2] = UART_parseInt(1000);
            UART_read();                         // .
            EspDrv__remoteIp[3] = UART_parseInt(1000);
            UART_read();                         // "
            UART_readBlocking();                 // ,
            EspDrv__remotePort = UART_parseInt(1000);// <remote port>

            UART_read();                         // :

            LOGDEBUG2(F("Data packet"), EspDrv__connId, EspDrv__bufPos);

            if(EspDrv__connId==socket || socket==0)
                return EspDrv__bufPos;
        }
    }
    return 0;
}

uint8_t EspDrv_getConnId() {
    return EspDrv__connId;
}

/*
 * It is mandatory to call EspDrv_availData() before calling this function.
 * It gets one byte of data, optionally picking without consuming the byte.
 * @return true on success, else false
 */
bool EspDrv_getData(uint8_t socket, uint8_t *data, bool peek, bool* connClose)
{

    long _startMillis;

    if (socket!=EspDrv__connId) {
        return false;
    }

    // see Serial.timedRead

    _startMillis = millis();
    do
    {
        if ( UART_available() > 0 )
        {
            if (peek)
            {
                *data = (char)UART_peek();
            }
            else
            {
                *data = (char)UART_read();
                EspDrv__bufPos--;
            }

            if (EspDrv__bufPos == 0)
            {
                // after the data packet a ",CLOSED" string may be received
                // this means that the socket is now closed

                delay(5);

                if ( UART_available() > 0 )
                {
                    //LOGDEBUG(".2");
                    //LOGDEBUG(espSerial->peek());

                    // 48 = '0'
                    if (UART_peek()==48+socket)
                    {
                        int idx = EspDrv_readUntil(500, ",CLOSED\n\r", false);
                        if(idx!=NUMESPTAGS)
                        {
                            LOGERROR(F("Tag CLOSED not found"));
                        }

                        LOGDEBUG("");
                        LOGDEBUG(F("Connection closed"));

                        *connClose=true;
                    }
                }
            }

            return true;
        }
    } while(millis() - _startMillis < 2000);

    // timed out, reset the buffer
    //orig LOGERROR1(F("TIMEOUT:"), _bufPos);
    LOGERROR(F("TIMEOUT:"));

    EspDrv__bufPos = 0;
    EspDrv__connId = 0;
    *data = 0;

    return false;
}

/**
 * It is mandatory to call EspDrv_availData() before calling this function.
 * Receive the data into a buffer.
 * It reads up to bufSize bytes.
 * @return received data size for success else -1.
 */
int EspDrv_getDataBuf(uint8_t socket, uint8_t *buf, uint16_t bufSize)
{

    int c;
    int i;

    if ( socket != EspDrv__connId ) {
        return 0;
    }

    if(EspDrv__bufPos<bufSize)
        bufSize = EspDrv__bufPos;

    for(i=0; i<bufSize; i++)
    {
        c = EspDrv_timedRead();
        //LOGDEBUG(c);
        if ( c == -1 ) {
            return -1;
        }

        *buf++ = (char)c;
        EspDrv__bufPos--;
    }

    return bufSize;
}


bool EspDrv_sendData(uint8_t socket, uint8_t *data, uint16_t len, bool appendCrLf)
{
    char cmdBuf[20];
    int idx;

    LOGDEBUG2(F("> sendData:"), socket, len);

    sprintf(cmdBuf, "AT+CIPSEND=%d,%u", socket, len + ( appendCrLf == true ? 2 : 0 ) );
    UART_println(cmdBuf);

    idx = EspDrv_readUntil(1000, (char *)">", false);
    if(idx!=NUMESPTAGS)
    {
        LOGERROR(F("Data packet send error (1)"));
        return false;
    }

    UART_write(data, len);

    if ( appendCrLf == true ) {
        UART_writeByte( '\n' );
        UART_writeByte( '\r' );
    }

    idx = EspDrv_readUntil(2000, NULL, true);
    if(idx!=TAG_SENDOK)
    {
        LOGERROR(F("Data packet send error (2)"));
        return false;
    }

    return true;
}

bool EspDrv_sendDataUdp(uint8_t socket, char* host, uint16_t port, uint8_t *data, uint16_t len)
{

    char cmdBuf[40];
    int idx;

    LOGDEBUG2(F("> sendDataUdp:"), socket, len);
    LOGDEBUG2ssi(F("> sendDataUdp:"), host, port);

    sprintf(cmdBuf, "AT+CIPSEND=%d,%u,\"%s\",%u", socket, len, host, port);
    //LOGDEBUG1(F("> sendDataUdp:"), cmdBuf);
    UART_println(cmdBuf);

    idx = EspDrv_readUntil(1000, (char *)">", false);
    if(idx!=NUMESPTAGS)
    {
        LOGERROR(F("Data packet send error (1)"));
        return false;
    }

    UART_write(data, len);

    idx = EspDrv_readUntil(2000, NULL, true);
    if(idx!=TAG_SENDOK)
    {
        LOGERROR(F("Data packet send error (2)"));
        return false;
    }

    return true;
}



void EspDrv_getRemoteIpAddress(IPAddress* ip)
{
    //orig ip = _remoteIp;
    IPAddress_copy( EspDrv__remoteIp, ip );
}

uint16_t EspDrv_getRemotePort()
{
    return EspDrv__remotePort;
}


////////////////////////////////////////////////////////////////////////////
// Utility functions
////////////////////////////////////////////////////////////////////////////



/*
* Sends the AT command and stops if any of the TAGS is found.
* Extract the string enclosed in the passed tags and returns it in the outStr buffer.
* Returns true if the string is extracted, false if tags are not found of timed out.
*/
bool EspDrv_sendCmdGet( char* cmd, char* startTag, char* endTag, char* outStr, int outStrLen)
{
    int idx;
    bool ret = false;

    outStr[0] = 0;

    EspDrv_espEmptyBuf( true );

    LOGDEBUG(F("----------------------------------------------"));
    LOGDEBUG1ss(F(">>"), cmd);

    // send AT command to ESP
    //orig espSerial->println(cmd);
    UART_println( cmd );

    // read result until the startTag is found
    idx = EspDrv_readUntil(1000, startTag, true);

    if(idx==NUMESPTAGS)
    {
        // clean the buffer to get a clean string
        RingBuffer_init();

        // start tag found, search the endTag
        idx = EspDrv_readUntil(500, endTag, true);

        if(idx==NUMESPTAGS)
        {
            // end tag found
            // copy result to output buffer avoiding overflow
            RingBuffer_getStrN(outStr, strlen(endTag), outStrLen-1);

            // read the remaining part of the response
            EspDrv_readUntil(2000, NULL, true);

            ret = true;
        }
        else
        {
            LOGWARN(F("End tag not found"));
        }
    }
#if _ESPLOGLEVEL_ > 3
    else if(idx>=0 && idx<NUMESPTAGS)
    {
        // the command has returned but no start tag is found
        LOGDEBUG1(F("No start tag found:"), idx);
    }
#endif
#if _ESPLOGLEVEL_ > 1
    else
    {
        // the command has returned but no tag is found
        LOGWARN(F("No tag found"));
    }
#endif

    LOGDEBUG1ss(F("---------------------------------------------- >"), outStr);
    LOGDEBUG("");

    return ret;
}


/*
* Sends the AT command and returns the id of the TAG.
* Return -1 if no tag is found.
*/
int EspDrv_sendCmd( char* cmd, int timeout)
{
    int idx;

    EspDrv_espEmptyBuf( true );

    LOGDEBUG(F("----------------------------------------------"));
    LOGDEBUG1ss(F(">>"), cmd);

    // orig espSerial->println(cmd);
    UART_println( cmd );

    idx = EspDrv_readUntil(timeout, NULL, true);

    LOGDEBUG1(F("---------------------------------------------- >"), idx);
    LOGDEBUG("");

    return idx;
}

/**
 * New function in the port. Substitutes sendCommand with variadic arguments.
 * Call sprintf( EspDrv_commandBuffer, ... ) prior to call this one.
 */
int EspDrv_sendCmdBuffer( int timeout )
{

    int idx;

    EspDrv_espEmptyBuf( true );

    LOGDEBUG(F("----------------------------------------------"));
    LOGDEBUG1ss(F(">>"), EspDrv_commandBuffer);

    //orig espSerial->println(cmdBuf);
    UART_println( EspDrv_commandBuffer );

    idx = EspDrv_readUntil(timeout, NULL, true);

    LOGDEBUG1(F("---------------------------------------------- >"), idx);
    LOGDEBUG("");

    return idx;

}

/*
* Sends the AT command and returns the id of the TAG.
* The additional arguments are formatted into the command using sprintf.
* Return -1 if no tag is found.
*/
/*//orig
int EspDrv::sendCmd(const __FlashStringHelper* cmd, int timeout, ...)
{
    char cmdBuf[CMD_BUFFER_SIZE];

    va_list args;
    va_start (args, timeout);
    vsnprintf_P (cmdBuf, CMD_BUFFER_SIZE, (char*)cmd, args);
    va_end (args);

    espEmptyBuf( true );

    LOGDEBUG(F("----------------------------------------------"));
    LOGDEBUG1(F(">>"), cmdBuf);

    espSerial->println(cmdBuf);

    int idx = readUntil(timeout, NULL, true);

    LOGDEBUG1(F("---------------------------------------------- >"), idx);
    LOGDEBUG();

    return idx;
}
*/

// Read from serial until one of the tags is found
// Returns:
//   the index of the tag found in the ESPTAGS array
//   -1 if no tag was found (timeout)
int EspDrv_readUntil(int timeout, char* tag, bool findTags)
{
    int ret;
    char c;
    int i;
    unsigned long start;

    LOGWARN("READ_UNTIL");

    RingBuffer_reset();

    start = millis();
    ret = -1;

    while ((millis() - start < timeout) && ret<0)
    {
        if ( UART_available() > 0 )
        {
            //LOGWARN("BYTE");

            c = (char)UART_read();

            //LOGDEBUG0c(c);

            RingBuffer_push(c);

            if (tag!=NULL)
            {
                if (RingBuffer_endsWith(tag))
                {
                    ret = NUMESPTAGS;
                    //LOGDEBUG1("xxx");
                }
            }
            if(findTags)
            {
                for(i=0; i<NUMESPTAGS; i++)
                {
                    if (RingBuffer_endsWith(ESPTAGS[i]))
                    {
                        ret = i;
                        break;
                    }
                }
            }
        }
    }

    if (ret < 0)
    {
        LOGWARN(F(">>> TIMEOUT >>>"));
    }

    return ret;
}


void EspDrv_espEmptyBuf(bool warn)
{
    char c;
    int i=0;
    long t = millis();

    while ( millis() - t > 2000 ) {
        while(UART_available() > 0) {
            c = UART_read();
#if _ESPLOGLEVEL_ > 3
            if (i>0 && warn==true)
                LOGDEBUG0c(c);
#endif
            i++;
            t = millis();
        }
    }
#if _ESPLOGLEVEL_ > 3
    if (i>0 && warn==true)
    {
        LOGDEBUG(F(""));
        LOGDEBUG1(F("Dirty characters in the serial buffer! >"), i);
    }
#endif

}


// copied from Serial::timedRead
int EspDrv_timedRead()
{
  int c;
  long _timeout = 10000;
  long _startMillis = millis();
  do
  {
    c = UART_read();
    if (c >= 0) return c;
  } while(millis() - _startMillis < _timeout);

  return -1; // -1 indicates timeout
}
