1. Arduino IDE vorbereiten

download: https://www.arduino.cc/en/software
sudo apt install esptool

chmod +x arduino-ide_*.AppImage
./arduino-ide_2.3.8_Linux_64bit.AppImage --no-sandbox

Board installieren (falls noch nicht):

Datei → Voreinstellungen → Zusätzliche Boardverwalter-URLs:
http://arduino.esp8266.com/stable/package_esp8266com_index.json

Datei → Einstellungen → "Ausführliche Ausgabe während: Kompilierung" ankreuzen → OK

Werkzeuge → Board → Boardverwalter → esp8266 installieren 3.2.1
Board wählen: LOLIN(WEMOS) D1 R2 & mini
Flash Size: 4MB (FS:2MB)

Werkzeuge - SSL Support → "Basic SSL ciphers (lower ROM, faster boot)"

Werkzeuge → Bibliotheken verwalten

Bibliothek          Version     Link
OneWire             2.3.7       https://github.com/PaulStoffregen/OneWire/archive/refs/tags/v2.3.7.zip
DallasTemperature   3.9.0       https://github.com/milesburton/Arduino-Temperature-Control-Library/archive/refs/tags/3.9.0.zip
PID                 1.2.1       https://github.com/br3ttb/Arduino-PID-Library
RCSwitch            2.6.4       https://github.com/sui77/rc-switch/archive/refs/tags/2.6.4.zip
ArduinoJson         6.21.5      https://github.com/bblanchon/ArduinoJson/archive/refs/tags/v6.21.5.zip

zeigt WeMo Port: ls /dev/ttyUSB* /dev/ttyACM*

esptool --port /dev/ttyUSB0 --baud 921600 --chip esp8266 write_flash 0x0 deine.bin

