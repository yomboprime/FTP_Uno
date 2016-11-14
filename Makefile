#    This file is part of ftpUno
# 
#    ftpUno  is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.



# Configure here your path to Node js:
node = /media/datos/soft/nodejs/node-v4.4.5-linux-x64/bin/node

# All the source files:
srcFiles = src/EspDrv/RingBuffer.c src/EspDrv/IPAddress.c src/EspDrv/EspDrv.c src/textUtils.c src/zxuno/zxuno.c src/zxuno/uart.c src/esxdos.c src/zxuno/ftp.c src/ftpuno.c
#srcFiles = src/textUtils.c src/zxuno/zxuno.c src/zxuno/esxdos.c src/ftpuno.c

# All the targets:
all: generateBASICLoader compile createTAP concatenateTAPs generateWavLeches


# Targets:

generateBASICLoader:
	./bas2tap/bas2tap -q -a10 -sftpUno ./cargadorBASIC/cargador.bas

compile:
	zcc +zx -o f.bin -lndos -lmzx $(srcFiles) > ultimolog.txt

createTAP:
	$(node) ./bin2tap-js/bin2tap.js ../f.bin > ultimolog.txt

concatenateTAPs:
	cat ./cargadorBASIC/cargador.tap f.tap > ftpUno.tap

generateWav:
	tape2wav ./ftpUno.tap ./ftpUno.wav > ultimolog.txt

generateWavLeches:
	./CgLeches ftpUno.tap ftpUno_LECHES.wav 3 > ultimolog.txt

clean:
	$(RM) *.bin *.i *.tap *.op* *.o *.reloc *~ zcc_opt.def *.wav ./cargadorBASIC/cargador.tap
