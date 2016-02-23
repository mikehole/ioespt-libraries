// 
// 
// 

#include "IoesptProvisioning.h"



IoesptProvisioning::IoesptProvisioning()
{
}

/////////////////////
//Settings

void IoesptProvisioning::loadSettings(JsonObject& root)
{
	DEBUG_WMS("Loading settings. wifiSettings: ");

	if (root.containsKey("wifiSettings"))
	{
		DEBUG_WMF("Found");

		JsonObject& wifiSettings = root["wifiSettings"];

		wifi.ssid = wifiSettings.get<String>("ssid");

		DEBUG_WMS("ssid : "); DEBUG_WMF(wifi.ssid);

		wifi.password = wifiSettings.get<String>("password");

		DEBUG_WMS("password : "); DEBUG_WMF(wifi.password);
	}
	else
	{
		DEBUG_WMF("Not Found");
	}
}

void IoesptProvisioning::saveSettings(JsonObject& root)
{
	JsonObject&	wifiSettings = root.createNestedObject("wifiSettings");

	wifiSettings.set<String>("ssid", wifi.ssid);
	wifiSettings.set<String>("password", wifi.password);
}

/////////////////////
//Portal

bool IoesptProvisioning::getConnected() 
{
	DEBUG_WMSL(F(""));

	if (wifi.ssid != "")
	{
		DEBUG_WMS(F("Using previously configured SSID:"));
		DEBUG_WMF(wifi.ssid);

		if (connectWifi() == WL_CONNECTED) {
			return true;
		}
	}
	
	server.reset(new ESP8266WebServer(80));


	start = millis();

	DEBUG_WMSL(F("Configuring access point... "));
	DEBUG_WMSL(_apName);

	if (_apPassword != NULL) {
		if (strlen(_apPassword) < 8 || strlen(_apPassword) > 63) {
			// fail passphrase to short or long!
			DEBUG_WMSL(F("Invalid AccessPoint password. Ignoring"));
			_apPassword = NULL;
		}
		DEBUG_WMSL(_apPassword);
	}

	//optional soft ip config
	if (_ap_static_ip) {
		DEBUG_WMSL(F("Custom AP IP/GW/Subnet"));
		WiFi.softAPConfig(_ap_static_ip, _ap_static_gw, _ap_static_sn);
	}

	if (_apPassword != NULL) {
		WiFi.softAP(_apName, _apPassword);//password option
	}
	else {
		WiFi.softAP(_apName);
	}

	delay(500); // Without delay I've seen the IP address blank

	DEBUG_WMSL(F("AP IP address: "));
	DEBUG_WMSL(WiFi.softAPIP());

	server->on("/listaccesspoints", std::bind(&IoesptProvisioning::listAccessPoints, this));

	server->on("/wifisettings", HTTP_GET,std::bind(&IoesptProvisioning::handleGetWifiSettings, this));

	server->on("/wifisettings", HTTP_POST, std::bind(&IoesptProvisioning::handleSetWifiSettings, this));

	server->begin(); // Web server start

	DEBUG_WMSL(F("HTTP server started"));

	while ( true  ) {
		//HTTP
		server->handleClient();

		if (connect)
		{
			connect = false;
			delay(2000);
			DEBUG_WMSL(F("Connecting to new AP"));

			// using user-provided  _ssid, _pass in place of system-stored ssid and pass
			if (connectWifi() != WL_CONNECTED) {
				DEBUG_WMSL(F("Failed to connect."));
			}
			else {
				//connected
				WiFi.mode(WIFI_STA);
				break;
			}
		}

		yield();
	}

	server.reset(); 

	return  WiFi.status() == WL_CONNECTED;
}

int IoesptProvisioning::connectWifi() {
	DEBUG_WMSL(F("Connecting as wifi client..."));

	// check if we've got static_ip settings, if we do, use those.
	if (_sta_static_ip) {
		DEBUG_WMSL(F("Custom STA IP/GW/Subnet"));
		WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn);
		DEBUG_WMSL(WiFi.localIP());
	}

	WiFi.begin(&wifi.ssid[0u], &wifi.password[0u]);

	int connRes = waitForConnectResult();
	DEBUG_WMS("Connection result: ");
	DEBUG_WMF(connRes);
	//not connected, WPS enabled, no pass - first attempt
	
	if (_tryWPS && connRes != WL_CONNECTED && wifi.password == "") {
		startWPS();
		//should be connected at the end of WPS
		connRes = waitForConnectResult();
	}

	return connRes;
}

uint8_t IoesptProvisioning::waitForConnectResult() {
	if (_connectTimeout == 0) {
		return WiFi.waitForConnectResult();
	}
	else {
		DEBUG_WMSL(F("Waiting for connection result with time out"));
		unsigned long start = millis();
		boolean keepConnecting = true;
		uint8_t status;
		while (keepConnecting) {
			status = WiFi.status();
			if (millis() > start + _connectTimeout) {
				keepConnecting = false;
				DEBUG_WMSL(F("Connection timed out"));
			}
			if (status == WL_CONNECTED || status == WL_CONNECT_FAILED) {
				keepConnecting = false;
			}
			delay(100);
		}
		return status;
	}
}

void IoesptProvisioning::startWPS() {
	DEBUG_WMSL("START WPS");
	WiFi.beginWPSConfig();
	DEBUG_WMSL("END WPS");
}

/////////////////////
//Web functions


void IoesptProvisioning::handleGetWifiSettings() 
{
	DEBUG_WMSL("handleGetWifiSettings");

	StaticJsonBuffer<1000> jsonBuffer;

	JsonObject& root = jsonBuffer.createObject();

	saveSettings(root);

	String out;
	int length = root.printTo(out);
	server->send(200, "text/json", out);
}

void IoesptProvisioning::handleSetWifiSettings() 
{
	DEBUG_WMSL("handleSetWifiSettings");

	StaticJsonBuffer<400> jsonBuffer;

	JsonObject& root = jsonBuffer.parseObject(server->arg("plain"));

	if (root.success())
	{
		wifi.ssid = root.get<String>("ssid");
		wifi.password = root.get<String>("password");

		if (settingsChanged != NULL)
			settingsChanged();

		server->send(200, "text/json", "{success:true}");

		connect = true;
	}
	else
		server->send(400, "text/json", "{error:'unable to parse JSON'}");
}


void IoesptProvisioning::listAccessPoints() {

	StaticJsonBuffer<2000> jsonBuffer;

	JsonObject& root = jsonBuffer.createObject();

	JsonArray& data = root.createNestedArray("WiFi");

	DEBUG_WMSL(F("List access points"));

	int n = WiFi.scanNetworks();
	DEBUG_WMSL(F("Scan done"));

	if (n == 0)
	{
		DEBUG_WMSL(F("No networks found"));
	}
	else
	{
		for (int i = 0; i < n; ++i)
		{
			JsonObject& wifi = jsonBuffer.createObject();

			DEBUG_WMS(WiFi.SSID(i));
			wifi["ssid"] = WiFi.SSID(i);

			DEBUG_WMC(" Power: ");
			DEBUG_WMF(WiFi.RSSI(i));

			int quality = getRSSIasQuality(WiFi.RSSI(i));

			wifi["quality"] = quality;

			data.add(wifi);
		}
	}

	char buffer[1024];
	root.printTo(buffer, sizeof(buffer));
	String json = buffer;
	server->send(200, "text/json", json);
}


int IoesptProvisioning::getRSSIasQuality(int RSSI) {
	int quality = 0;

	if (RSSI <= -100) {
		quality = 0;
	}
	else if (RSSI >= -50) {
		quality = 100;
	}
	else {
		quality = 2 * (RSSI + 100);
	}
	return quality;
}


///////////////////////////
// Diagnostics

template <typename Generic>
void IoesptProvisioning::DEBUG_WMSL(Generic text) {
	Serial.print("*IOESPT-Provisioning: ");
	Serial.println(text);
}

template <typename Generic>
void IoesptProvisioning::DEBUG_WMS(Generic text) {
	Serial.print("*IOESPT-Provisioning: ");
	Serial.print(text);
}

template <typename Generic>
void IoesptProvisioning::DEBUG_WMC(Generic text) {
	Serial.print(text);
}

template <typename Generic>
void IoesptProvisioning::DEBUG_WMF(Generic text) {
	Serial.println(text);
}