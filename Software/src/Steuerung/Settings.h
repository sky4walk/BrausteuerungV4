#ifndef __SETTINGS__
#define __SETTINGS__

#define SSID_ADDR	0
#define SSID_SIZE	32
#define PASS_ADDR	SSID_ADDR + SSID_SIZE
#define PASS_SIZE	64

class Settings {
public:	
	Settings() {
		
	}
	void init() {
		EEPROM.begin(512);
	}
	String getSSID() {
		String esid = "";
		for (int i = SSID_ADDR; i < SSID_SIZE; ++i) {
			esid += char(EEPROM.read(i));
		}
		return esid;
	}
	String getPASS() {
		String pass = "";
		for (int i = PASS_ADDR; i < PASS_SIZE; ++i) {
			pass += char(EEPROM.read(i));
		}
		return pass;
	}
	void setWifi(String& ssid, String& pass) {
		if (ssid.length() > 0 && pass.length() > 0) {
			for (int i = 0; i < SSID_SIZE; ++i) {
				EEPROM.write(SSID_ADDR+i, 0);
			}
			for (int i = 0; i < PASS_SIZE; ++i) {
				EEPROM.write(PASS_ADDR+i, 0);
			}
			for (int i = 0; i < ssid.length() && i < SSID_SIZE; ++i) {
				EEPROM.write(SSID_ADDR+i, ssid[i]);
			}
			for (int i = 0; i < pass.length() && i < PASS_SIZE; ++i) {
				EEPROM.write(PASS_ADDR+i, pass[i]);
			}
			EEPROM.commit();
		}
	}
};
#endif
