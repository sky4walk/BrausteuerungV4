#ifndef __SETTINGS__
#define __SETTINGS__

#define SSID_ADDR	0
#define SSID_SIZE	32
#define PASS_ADDR	(SSID_ADDR+SSID_SIZE)
#define PASS_SIZE	64

class Settings {
  public:
    Settings() {

    }
    void init() {
      EEPROM.begin(512);
    }
    void deleteAll() {
      delSSID();
      delPASS();
    }
    String getSSID() {
      String esid = "";
      for (int i = SSID_ADDR; i < SSID_SIZE; ++i) {
        char c = (char)EEPROM.read(i);
        if ( 0 != c )
          esid += String(c);
      }
      return esid;
    }
    void delSSID() {
        for (int i = 0; i < SSID_SIZE; ++i) {
          EEPROM.write(SSID_ADDR + i, 0);
        }
        EEPROM.commit();      
    }
    void setSSID(String& ssid) {
      delSSID();
      if (ssid.length() > 0) {
        for (int i = 0; i < ssid.length() && i < SSID_SIZE; ++i) {
          EEPROM.write(SSID_ADDR + i, ssid[i]);
        }
        EEPROM.commit();      
      }
    }
    String getPASS() {
      String pass = "";
      for (int i = PASS_ADDR; i < PASS_SIZE; ++i) {
        char c = (char)EEPROM.read(i);
        if ( 0 != c )
          pass += String(c);
      }
      return pass;
    }
    void delPASS() {
      for (int i = 0; i < PASS_SIZE; ++i) {
        EEPROM.write(PASS_ADDR + i, 0);
      }
      EEPROM.commit();
    }
    void setPASS(String& pass) {
      delPASS();
      for (int i = 0; i < pass.length() && i < PASS_SIZE; ++i) {
        EEPROM.write(PASS_ADDR + i, pass[i]);
      }
      EEPROM.commit();
    }
};
#endif
