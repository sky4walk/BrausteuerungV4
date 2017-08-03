#ifndef __WIFICONNECT__
#define __WIFICONNECT__

#include "settings.h"

#define MY_SSID "mikroSikaru.de"

class WifiConnect {
public:	
	WifiConnect(Settings& data) 
		: mData(data) {		
	}
	void init() {
	}
	
	bool testWifi() {
		String ssid = mData.getSSID();
		String pass = mData.getPASS();
		if ( ssid.length() > 1 ) {
			int c = 0;
			WiFi.begin(ssid.c_str(), pass.c_str());
			while ( c < 20 ) {
				delay(500);
				if (WiFi.status() == WL_CONNECTED)
					return true;
				c++;
			}
		}
		return false;
	}
	
	void setupAP() {
		//WiFi.disconnect();
		IPAddress myIP(10, 0, 0, 1);
		IPAddress subnet(255, 255, 255, 0);
		WiFi.mode(WIFI_AP_STA);
		WiFi.config(myIP, myIP, subnet);
		WiFi.softAP(MY_SSID);
		delay(2000);
	}
private:	
	Settings& mData;
};

#endif
