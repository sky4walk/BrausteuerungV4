SET COMPORT=COM6
SET ESPTOOLSPATH=..\sw\EspTools
%ESPTOOLSPATH%\esptool.exe -vv -cd nodemcu -cb 115200 -cp %COMPORT% -ca 0x00000 -cf Blink.ino.bin 