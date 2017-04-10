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

#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include "integerTypes.h"

#define PARAM_NAME_LENGTH 32
#define PARAM_VALUE_LENGTH 128

#define CONFIG_OK -1
#define CONFIG_ERROR_NOTFOUND -2
#define CONFIG_ERROR_READING -3
#define CONFIG_ERROR_CANTCLOSE -4
#define CONFIG_ERROR_VALIDATION -5

extern int loadConfigFile( uint8_t *configFilePath, void *paramCallback, uint8_t *fileBuffer, uint16_t bufferSize );
extern uint8_t *configError( int error );

#endif /* CONFIG_H */
