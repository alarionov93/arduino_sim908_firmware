sudo avrdude -b 19200 -c ftdi -p m328p -P /dev/ttyUSB0 -U flash:w:/tmp/arduino_build_470063/sim908_firmware.ino.hex
