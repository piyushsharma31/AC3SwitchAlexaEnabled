/*

AC Switch with 3 switches

Inspired from:

AC Voltage dimmer with Zero cross detection
Author: Charith Fernanado http://www.inmojo.com
License: Creative Commons Attribution Share-Alike 3.0 License.

*/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <EepromUtil.h>
#include <ESPConfig.h>
#include <ESP8266Controller.h>
#include "ACSwitch.h"
#include "UpnpBroadcastResponder.h"
#include "CallbackFunction.h"

ESPConfig configuration(/*controller name*/  "AC 3 Switch", /*location*/ "Unknown", /*firmware version*/ "ac3s.200919.bin", /*router SSID*/ "onion", /*router SSID key*/ "242374666");
WiFiUDP Udp;
int POWER_PIN = 10;/*fake pin, does not disturb the switch state*/

byte packetBuffer[255 * 3];
byte replyBuffer[255 * 3];
short replyBufferSize = 0;

UpnpBroadcastResponder upnpBroadcastResponder;
boolean udpMulticastMode = false;

// initialize dimmer, switch objects
ACSwitch sswitch1("SWITCH ONE", 0/*AC load pin*/, 2/*No. of capabilities*/, configuration.sizeOfEEPROM());
ACSwitch sswitch2("SWITCH TWO", 2/*AC load pin*/, 2/*No. of capabilities*/, configuration.sizeOfEEPROM() + sswitch1.sizeOfEEPROM());
ACSwitch sswitch3("SWITCH THREE", 3/*AC load pin*/, 2/*No. of capabilities*/, configuration.sizeOfEEPROM() + sswitch1.sizeOfEEPROM() + sswitch2.sizeOfEEPROM());

void setup() {

	//delay(1000);
	Serial.begin(115200);//,SERIAL_8N1,SERIAL_TX_ONLY);

	Serial.println("Setting up devices for Android app discovery");

	configuration.init(POWER_PIN);

	// load controller capabilities (values) from EEPROM
	sswitch1.loadCapabilities();
	sswitch2.loadCapabilities();
	sswitch3.loadCapabilities();

	Serial.println("Setting up device for Alexa discovery");
	upnpBroadcastResponder.beginUdpMulticast();

	sswitch1.enableAlexaDiscovery(sswitch1.controllerName, 80, sswitch1On, sswitch1Off, sswitch1State);
	sswitch2.enableAlexaDiscovery(sswitch2.controllerName, 81, sswitch2On, sswitch2Off, sswitch2State);
	sswitch3.enableAlexaDiscovery(sswitch3.controllerName, 82, sswitch3On, sswitch3Off, sswitch3State);

	upnpBroadcastResponder.addDevice(*sswitch1.alexaDevice);
	upnpBroadcastResponder.addDevice(*sswitch2.alexaDevice);
	upnpBroadcastResponder.addDevice(*sswitch3.alexaDevice);

	Udp.begin(port);
}

unsigned long last = 0;
int packetSize = 0;
_udp_packet udpPacket;
int heap_print_interval = 10000;

void loop() {

	/*if (millis() - last > heap_print_interval) {
		DEBUG_PRINTLN();

		last = millis();
		DEBUG_PRINT("[MAIN] Free heap: ");
		Serial.println(ESP.getFreeHeap(), DEC);

	}*/

	upnpBroadcastResponder.serverLoop();
	sswitch1.alexaDevice->serverLoop();
	sswitch2.alexaDevice->serverLoop();
	sswitch3.alexaDevice->serverLoop();

	packetSize = Udp.parsePacket();

	if (packetSize) {

		DEBUG_PRINT("Received packet of size "); DEBUG_PRINT(packetSize); DEBUG_PRINT(" from "); DEBUG_PRINT(Udp.remoteIP()); DEBUG_PRINT(", port ");
		DEBUG_PRINTLN(Udp.remotePort());

		// read the packet into packetBuffer
		Udp.read(packetBuffer, packetSize);
		packetBuffer[packetSize] = 0;

		// initialize the replyBuffer
		memcpy(replyBuffer, packetBuffer, 3);

		udpPacket._size = packetBuffer[1] << 8 | packetBuffer[0];
		udpPacket._command = packetBuffer[2];
		udpPacket._payload = (char*)packetBuffer + 3;

		if (udpPacket._command == DEVICE_COMMAND_DISCOVER) {

			memcpy(replyBuffer, packetBuffer, 3);
			replyBufferSize = 3 + configuration.discover(replyBuffer+3);

		} else if (udpPacket._command == DEVICE_COMMAND_SET_CONFIGURATION) {

			memcpy(replyBuffer, packetBuffer, 3);
			replyBufferSize = 3 + configuration.set(replyBuffer+3, (byte*)udpPacket._payload);

		} else if (udpPacket._command == DEVICE_COMMAND_GET_CONTROLLER) {
			DEBUG_PRINTLN("command = DEVICE_COMMAND_GET_CONTROLLER");

			memcpy(replyBuffer, packetBuffer, 3);

			byte _pin = udpPacket._payload[0];
			if (_pin == sswitch1.pin) {

				memcpy(replyBuffer + 3, sswitch1.toByteArray(), sswitch1.sizeOfUDPPayload());
				replyBufferSize = 3 + sswitch1.sizeOfUDPPayload();

			} else if (_pin == sswitch2.pin) {

				memcpy(replyBuffer + 3, sswitch2.toByteArray(), sswitch2.sizeOfUDPPayload());
				replyBufferSize = 3 + sswitch2.sizeOfUDPPayload();

			} else if (_pin == sswitch3.pin) {

				memcpy(replyBuffer + 3, sswitch3.toByteArray(), sswitch3.sizeOfUDPPayload());
				replyBufferSize = 3 + sswitch3.sizeOfUDPPayload();

			}

		} else if (udpPacket._command == DEVICE_COMMAND_SET_CONTROLLER) {

			byte _pin = udpPacket._payload[0];

			// (OVERRIDE) send 3 bytes (size, command) as reply to client
			replyBufferSize = 3;

			if (_pin == sswitch1.pin) {

				sswitch1.fromByteArray((byte*)udpPacket._payload);

			} else if (_pin == sswitch2.pin) {

				sswitch2.fromByteArray((byte*)udpPacket._payload);

			} else if (_pin == sswitch3.pin) {

				sswitch3.fromByteArray((byte*)udpPacket._payload);

			}

		} else if (udpPacket._command == DEVICE_COMMAND_GETALL_CONTROLLER) {

			memcpy(replyBuffer + 3, sswitch1.toByteArray(), sswitch1.sizeOfUDPPayload());
			memcpy(replyBuffer + 3 + sswitch1.sizeOfUDPPayload(), sswitch2.toByteArray(), sswitch2.sizeOfUDPPayload());
			memcpy(replyBuffer + 3 + sswitch1.sizeOfUDPPayload() + sswitch2.sizeOfUDPPayload(), sswitch3.toByteArray(), sswitch3.sizeOfUDPPayload());

			replyBufferSize = 3 + sswitch1.sizeOfUDPPayload() + sswitch2.sizeOfUDPPayload() + sswitch3.sizeOfUDPPayload();

		} else if (udpPacket._command == DEVICE_COMMAND_SETALL_CONTROLLER) {

			int i = 0;

			// update the LED variables with new values
			for (int count = 0; count < 3; count++) {

				if (udpPacket._payload[i] == sswitch1.pin) {

					sswitch1.fromByteArray((byte*)udpPacket._payload + i);

					i = i + sswitch1.sizeOfEEPROM();

				} else if (udpPacket._payload[i] == sswitch2.pin) {

					sswitch2.fromByteArray((byte*)udpPacket._payload + i);

					i = i + sswitch2.sizeOfEEPROM();

				} else if (udpPacket._payload[i] == sswitch3.pin) {

					sswitch3.fromByteArray((byte*)udpPacket._payload + i);

					i = i + sswitch3.sizeOfEEPROM();

				}
			}

			// (OVERRIDE) send 3 bytes (size, command) as reply to client
			replyBufferSize = 3;
		}

		// update the size of replyBuffer in packet bytes
		replyBuffer[0] = lowByte(replyBufferSize);
		replyBuffer[1] = highByte(replyBufferSize);

		// send a reply, to the IP address and port that sent us the packet we received
		Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
		Udp.write(replyBuffer, replyBufferSize);
		Udp.endPacket();
	}

	sswitch1.loop();
	sswitch2.loop();
	sswitch3.loop();

	yield();
}


int sswitch1On() {
	Serial.println("Switch 1 turn on ...");
	sswitch1.capabilities[0]._value  = 1;
	return sswitch1.capabilities[0]._value;
}

int sswitch1Off() {
	Serial.println("Switch 1 turn off ...");
	sswitch1.capabilities[0]._value  = 0;
	return sswitch1.capabilities[0]._value;
}

int sswitch1State() {
	Serial.println("Switch 1 get state ...");
	return (sswitch1.capabilities[0]._value);
}

int sswitch2On() {
	Serial.println("Switch 2 turn on ...");
	sswitch2.capabilities[0]._value  = 1;
	return sswitch2.capabilities[0]._value;
}

int sswitch2Off() {
	Serial.println("Switch 2 turn off ...");
	sswitch2.capabilities[0]._value = 0;
	return sswitch2.capabilities[0]._value;
}

int sswitch2State() {
	Serial.println("Switch 2 get state ...");
	return (sswitch2.capabilities[0]._value);
}

int sswitch3On() {
	Serial.println("Switch 3 turn on ...");
	sswitch3.capabilities[0]._value  = 1;
	return sswitch3.capabilities[0]._value;
}

int sswitch3Off() {
	Serial.println("Switch 3 turn off ...");
	sswitch3.capabilities[0]._value  = 0;
	return sswitch3.capabilities[0]._value;
}

int sswitch3State() {
	Serial.println("Switch 3 get state ...");
	return (sswitch3.capabilities[0]._value);
}
