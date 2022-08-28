#ifndef SWITCH_H
#define SWITCH_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUDP.h>
#include "CallbackFunction.h"

class AlexaDevice {
private:
	ESP8266WebServer *server = NULL;
	WiFiUDP UDP;
	String serial;
	String persistent_uuid;
	String device_name;
	unsigned int port;
	CallbackFunction onCallback;
	CallbackFunction offCallback;
	CallbackFunction statusCallback;
	int switchStatus;

	void startWebServer();
	void handleEventservice();
	void handleUpnpControl();
	void handleRoot();
	void handleSetupXml();
public:
	AlexaDevice();
	AlexaDevice(String alexaInvokeName, unsigned int port, CallbackFunction onCallback, CallbackFunction offCallback, CallbackFunction statusCallback);
	~AlexaDevice();
	String getAlexaInvokeName();
	void serverLoop();
	void respondToSearch(IPAddress& senderIP, unsigned int senderPort);
	void sendRelayState();
};

#endif
