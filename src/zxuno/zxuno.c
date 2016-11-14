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

#include "zxuno.h"

#include <time.h>

long ZX_Uno_t = 0UL;

long millis() {

    // Note: this is a not exact simplification
    // Should be ticks(50 Hz) * 20 ms = 1000 ms,
    // But it is simplified to ticks(50 Hz) * 16 = 800 ms
    // 50 Hz * 16 = 800 ~ 1000 ms / s
    return time( &ZX_Uno_t ) << 4;

}
