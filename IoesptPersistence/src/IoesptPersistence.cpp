// 
// 
// 

#include "IoesptPersistence.h"

IoesptPersistance::IoesptPersistance()
{
}

void IoesptPersistance::loadSettings(LoadCallbackType callback)
{
	EEPROM.begin(BufferLen + 2);

	StaticJsonBuffer<1000> jsonBuffer;

	int bufferLength = word(EEPROM.read(0), EEPROM.read(1));

	DEBUG_WMS("Eprom contents length:");
	DEBUG_WMF(bufferLength);


	int address = 2;

	for (address = 2; address < bufferLength + 2; address++) {
		buffer[address - 2] = EEPROM.read(address);
	}

	buffer[address - 2] = '\0';
	
	DEBUG_WMSL("Buffer Contents:");
	DEBUG_WMF(buffer);

	JsonObject& root = jsonBuffer.parseObject(buffer);

	if (root.success())
		callback(root);
	else
		DEBUG_WMSL("Failed to read settings JSON.");
	
}

void IoesptPersistance::saveSettings(SaveCallbackType callback)
{
	EEPROM.begin(BufferLen + 2);
	
	StaticJsonBuffer<2000> jsonBuffer;

	JsonObject& root = jsonBuffer.createObject();
	
	callback(root);

	root.prettyPrintTo(Serial);

	int bufferLength = root.printTo(buffer, length);

	DEBUG_WMS("Saving Eprom contents length:");
	DEBUG_WMF(length);
	
	root.prettyPrintTo(Serial);
	DEBUG_WMF("");

	EEPROM.write(0, highByte(bufferLength));
	EEPROM.write(1, lowByte(bufferLength));
	for (int address = 2; address < bufferLength + 2; address++) {
		EEPROM.write(address, buffer[address - 2]);
	}
	EEPROM.commit();
}

///////////////////////////
// Diagnostics

template <typename Generic>
void IoesptPersistance::DEBUG_WMSL(Generic text) {
	Serial.print("*IOESPT-Settings: ");
	Serial.println(text);
}

template <typename Generic>
void IoesptPersistance::DEBUG_WMS(Generic text) {
	Serial.print("*IOESPT-Settings: ");
	Serial.print(text);
}

template <typename Generic>
void IoesptPersistance::DEBUG_WMC(Generic text) {
	Serial.print(text);
}

template <typename Generic>
void IoesptPersistance::DEBUG_WMF(Generic text) {
	Serial.println(text);
}
