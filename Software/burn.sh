#!/bin/bash

cd Default

avr-objcopy -j .text -j .data -O ihex hymdigi hymdigi.hex
avr-objcopy -j .text -j .data -O binary hymdigi hymdigi.bin

# Update this parameters accordingly to your setup.
# copy correct file to burn
cp Build/firmware.hex ./

#avrdude -v -carduino -pATMEGA328P -P/dev/ttyUSB0 -b115200 -D -Uflash:w:hymdigi.hex
avrdude -v -c usbasp -pATMEGA328P -u -U flash:w:firmware.hex 
