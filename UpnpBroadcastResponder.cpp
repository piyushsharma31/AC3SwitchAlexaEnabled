#include "UpnpBroadcastResponder.h"
#include "AlexaDevice.h"
#include <functional>

// Multicast declarations
IPAddress ipMulti(239, 255, 255, 250);
const unsigned int portMulti = 1900;
char upnpPacketBuffer[UDP_TX_PACKET_MAX_SIZE];

#define MAX_SWITCHES 14
AlexaDevice switches[MAX_SWITCHES] = {};
int numOfSwitchs = 0;

//#define numOfSwitchs (sizeof(switches)/sizeof(AlexaDevice)) //array size  

//<<constructor>>
UpnpBroadcastResponder::UpnpBroadcastResponder(){/*nothing to construct*/}

//<<destructor>>
UpnpBroadcastResponder::~UpnpBroadcastResponder(){/*nothing to destruct*/}

bool UpnpBroadcastResponder::beginUdpMulticast(){
	boolean state = false;

	Serial.println("Begin multicast ..");

	if(UDP.beginMulticast(WiFi.localIP(), ipMulti, portMulti)) {
		Serial.print("Udp multicast server started at ");
		Serial.print(ipMulti);
		Serial.print(":");
		Serial.println(portMulti);

		state = true;
	}
	else{
		Serial.println("Connection failed");
	}

	return state;
}

void UpnpBroadcastResponder::addDevice(AlexaDevice& device) {
	Serial.print("Adding switch : ");
	Serial.print(device.getAlexaInvokeName());
	Serial.print(" index : ");
	Serial.println(numOfSwitchs);

	switches[numOfSwitchs] = device;
	numOfSwitchs++;
}

void UpnpBroadcastResponder::serverLoop() {
	int packetSize = UDP.parsePacket();
	if (packetSize <= 0)
	return;

	IPAddress senderIP = UDP.remoteIP();
	unsigned int senderPort = UDP.remotePort();

	// read the packet into the buffer
	UDP.read(upnpPacketBuffer, packetSize);

	// check if this is a M-SEARCH for WeMo device
	String request = String((char *)upnpPacketBuffer);
	//String request = String((char *)buffer);

	if(request.indexOf("M-SEARCH") >= 0) {
		// Issue https://github.com/kakopappa/arduino-esp8266-alexa-multiple-wemo-switch/issues/22 fix
		if((request.indexOf("urn:Belkin:device:**") > 0) || (request.indexOf("ssdp:all") > 0) || (request.indexOf("upnp:rootdevice") > 0)) {
			Serial.println("Got UDP Belkin Request..");
			
			// int arrSize = sizeof(switchs) / sizeof(AlexaDevice);
			
			for(int n = 0; n < numOfSwitchs; n++) {
				AlexaDevice &sw = switches[n];

				if (&sw != NULL) {
					sw.respondToSearch(senderIP, senderPort);              
				}
			}
		}
	}
}
