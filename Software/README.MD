hymDIGI - APRS Digipeater for Arduino
----------------------------------------

Communication layers are based on BeRTOS

BeRTOS is Copyright Develer S.r.l. (http://www.develer.com/).





Preamble length (TXdelay): CONFIG_AFSK_PREAMBLE_LEN cfg_afsk.h
avrdude -v -c arduino -p ATMEGA328P -P /dev/ttyUSB0 -b 115200 -D -U flash:w:firmware.hex

