../../../../sw/EspTools/esptool-v4.6.2-win64/esptool.exe --chip esp8266 --port COM4 --baud 115200 --before default_reset --after hard_reset write_flash 0x0 hw_test.ino.bin
