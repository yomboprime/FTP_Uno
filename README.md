# FTP-Uno
FTP Client for ZX-Uno using the WiFi ESP-12E addon.

## Quick Start

- Upgrade your ZX-Uno by writting in it the [ZX core with UART support](https://github.com/yomboprime/zxuno-addons/blob/master/test24_uart_dac_vga/v4/COREn.ZX1?raw=true)
- Plug your WiFi addon to your ZX-Uno. Each time you are going to use it, remember to select the correct core in the Core Selection Menu of your ZX-Uno (it shows up at boot pressing Caps Lock key)
- If you have not done so yet, run the [WiFi addon configuration utility](https://github.com/yomboprime/ZXYLib/blob/master/WIFICONF.tap?raw=true) to enable your ESP-12E WiFi module for the ZX-Uno.
- Edit the text file `FTP.CFG` to suit your configuration. It has self-explanatory instructions (for more info read the last section)
- Copy the edited file `FTP.CFG` into the directory `/SYS/CONFIG/` of your SD card.
- Copy the file `ftpUno.tap` wherever you want into the SD card.
- Start your ZX-Uno and run the `ftpUno.tap` program file by NMI event or other method.
- The program will start to connect to the WiFi and then to the FTP server. The status message will show connection information.

## Interface

The screen is divided into left view (the FTP file list view), right view (the SD card file list view), and a lower message bar that displays current status and confirmation messages.

Each view has a scrolling bar to its right that shows what portion of the file list is visible right now.

![Screenshot](https://github.com/yomboprime/FTP_Uno/raw/master/screenshots/FTPscreenshot.jpg)

## Input

The program is controlled with the keyboard, using the following keys:

- Cursor UP: Select upper file, or go to previous page if there are more files scrolling up.
- Cursor DOWN: Select lower file, or go to next page if there are more files scrolling down.
- Cursor LEFT/RIGHT: Change selection between FTP or SD card view.
- INTRO: Enter the selected directory, or download selected file (in the FTP view), or delete the selected file (in the SD card view)
- BACKSPACE: Go up one directory.

## Configuration file

The program is completely configured through its configuration file located in the directory `/SYS/CONFIG/FTP.CFG`. This file must exist prior to executing the program.

The content is a series of `param=value` pairs, one pair in each line. Put `#` For one-line comments. The maximum value length is 128 characters.

Accepted parameters names follows (all are mandatory):

- `ssid`: Name of the WiFi network the program is going to connect to (i.e. network SSID)
- `wifi_password`: WiFi password. Must not be empty, even for not secure networks (put whatever in that case)
- `server`: The IP address or host name of the FTP Server the program is going to connect to. Example: `12.34.56.78` or `files.myserver.org`
- `port`: Server port. Usually `21` for FTP.
- `user`: FTP user name. Usually `anonymous`
- `ftp_password`: FTP user password. Put whatever if the user is `anonymous`.
- `ftp_initial_dir`: Initial path in the FTP server. Usually `/`. Must end with `/`.
- `sd_initial_dir`: Initial path in the SD card. Usually `/`. Must end with `/`.
