#ifndef __WIFICONNECT__
#define __WIFICONNECT__

#include "settings.h"

class WifiConnect {
	WifiConnect(Settings& data) 
		: mData(data) {		
	}
	init() {
	}
	Settings& mData;
};

#endif