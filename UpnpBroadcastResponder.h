#ifndef UPNPBROADCASTRESPONDER_H
#define UPNPBROADCASTRESPONDER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include "AlexaDevice.h"

class UpnpBroadcastResponder {
private:
	WiFiUDP UDP;  
public:
	UpnpBroadcastResponder();
	~UpnpBroadcastResponder();
	bool beginUdpMulticast();
	void serverLoop();
	void addDevice(AlexaDevice& device);
};

#endif
