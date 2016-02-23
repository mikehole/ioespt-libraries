// IoesptProvisioning.h

#ifndef _IOESPTPROVISIONING_h
#define _IOESPTPROVISIONING_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

struct WifiConfig {
	String ssid = "";
	String password = "";
};

typedef void(*SettingsChangedCallbackType)();

class IoesptProvisioning
{
public:
	WifiConfig wifi;

	IoesptProvisioning();

	bool getConnected();

	void loadSettings(JsonObject& root);

	void saveSettings(JsonObject& root);

	SettingsChangedCallbackType settingsChanged;

private:

	unsigned long _configPortalTimeout = 0;
	unsigned long _connectTimeout = 0;
	unsigned long _configPortalStart = 0;
	boolean       _tryWPS = false;

	bool connect = false;

	int connectWifi();

	uint8_t waitForConnectResult();

	void startWPS();

	//Web server handlers
	void handleGetWifiSettings();

	void handleSetWifiSettings();

	void listAccessPoints();

	//Utility
	int getRSSIasQuality(int RSSI);

	std::unique_ptr<ESP8266WebServer> server;

	unsigned long timeout = 0;
	unsigned long start = 0;

	const char*   _apName = "iotespt";
	const char*   _apPassword = NULL;

	IPAddress     _ap_static_ip;
	IPAddress     _ap_static_gw;
	IPAddress     _ap_static_sn;
	IPAddress     _sta_static_ip;
	IPAddress     _sta_static_gw;
	IPAddress     _sta_static_sn;

	//Diagnostics
	template <typename Generic>
	void DEBUG_WMSL(Generic text);

	template <typename Generic>
	void DEBUG_WMS(Generic text);

	template <typename Generic>
	void DEBUG_WMC(Generic text);

	template <typename Generic>
	void DEBUG_WMF(Generic text);

};


#endif

