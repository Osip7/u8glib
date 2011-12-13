#!/bin/sh
#
# arg 1: pt 
# arg 2: ttf name
# arg 3: internalname
#
PT=$1
ARG3=$3
BDF=${ARG3}.bdf
CFILE=u8g_font_${ARG3}.c
IDENTIFIER=u8g_font_${ARG3}
echo $2  ... ${CFILE}
../otf2bdf/otf2bdf -a -p $1 -r 72 $2 -o ${BDF}
../bdf2u8g/bdf2u8g -b 42 -e 57 ${BDF} ${IDENTIFIER} ${CFILE} >/dev/null
#cp ${CFILE} ../../../src/.
