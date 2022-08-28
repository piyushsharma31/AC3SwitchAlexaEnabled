#ifndef ACSwitch_h
#define ACSwitch_h
#include "CallbackFunction.h"
#include "AlexaDevice.h"

class ACSwitch : public ESP8266Controller {

public:
	// store last time BULB was updated (blink)
	unsigned long blinkPreviousMillis = 0;

	short __ON = LOW;
	short __OFF = HIGH;

public:
	ACSwitch(char* nam, uint8_t _pin, uint8_t capCount, int start_address):ESP8266Controller(nam, _pin, capCount, start_address)
	{

		pinMode(pin, OUTPUT);

		// On and Off switch
		strcpy(capabilities[0]._name, "ON/OFF");
		capabilities[0]._value_min = 0;
		capabilities[0]._value_max = 1;
		capabilities[0]._value  = 0;

		// Toggle the switch (on/off) at an interval (blinkDelay)
		strcpy(capabilities[1]._name, "BLINK");
		capabilities[1]._value_min = 0;
		capabilities[1]._value_max = 5000;
		capabilities[1]._value  = 0;

	}

public:
	virtual void loop();
	virtual void enableAlexaDiscovery(String alexaInvokeName, unsigned int port, CallbackFunction onCallback, CallbackFunction offCallback, CallbackFunction statusCallback);

	// Alexa discovery device
	AlexaDevice* alexaDevice=0;
	//boolean alexaDeviceAdded = false;

};

void ACSwitch::enableAlexaDiscovery(String alexaInvokeName, unsigned int port, CallbackFunction onCallback, CallbackFunction offCallback, CallbackFunction statusCallback) {

	if(alexaDevice==0) {
		//alexaDeviceAdded = true;
		alexaDevice = new AlexaDevice(alexaInvokeName, port, onCallback, offCallback, statusCallback);
	}
}

void ACSwitch::loop() {

	// check to see if it's time to change the state of the BULB
	unsigned long currentMillis = millis();

	if (capabilities[1]._value  == 0) {				// blinking==OFF

		if (capabilities[0]._value  == 0			// onOff==Off
				&& pinState == __ON ) {				// LED is currently ON

			// BULB should be ALWAYS OFF in these conditions
			pinState = __OFF;
			digitalWrite(pin, pinState);

			DEBUG_PRINT(currentMillis / 1000); DEBUG_PRINT(" ACSwitch "); toString();

		} else if(capabilities[0]._value  == 1			// onOff==On
				&& pinState == __OFF ) {					// LED is currently OFF

			// BULB should be ALWAYS ON in these conditions
			pinState = __ON;
			digitalWrite(pin, pinState);

			DEBUG_PRINT(currentMillis / 1000); DEBUG_PRINT(" ACSwitch "); toString();
		}

	} else if (capabilities[1]._value  > 0          // blinking==ON
			&& capabilities[0]._value  == 1) {     // onOff==On

		if ( currentMillis - blinkPreviousMillis >= capabilities[1]._value) {// blink delay over

			if (pinState == __OFF) {
				// BULB should turn on
				pinState = __ON;
				digitalWrite(pin, pinState);
				DEBUG_PRINT(currentMillis / 1000); DEBUG_PRINT(" ACSwitch "); toString();

			} else if (pinState == __ON) {
				// BULB should turn off
				pinState = __OFF;
				digitalWrite(pin, pinState);
				DEBUG_PRINT(currentMillis / 1000); DEBUG_PRINT(" ACSwitch "); toString();
			}

			// Remember the time
			blinkPreviousMillis = currentMillis;

		}
	} else if (capabilities[1]._value  > 0			// blinking==ON
			&& capabilities[0]._value  == 0) {		// onOff==Off

		if(pinState==__ON) {
			// BULB should be ALWAYS OFF in these conditions
			pinState = __OFF;
			digitalWrite(pin, pinState);

			DEBUG_PRINT(currentMillis / 1000); DEBUG_PRINT(" ACSwitch "); toString();
		}

	}


	// update EEPROM if capabilities changed
	if (currentMillis - lastEepromUpdate > eeprom_update_interval) {
		//DEBUG_PRINTLN();

		lastEepromUpdate = millis();
		//DEBUG_PRINT("[MAIN] Free heap: ");
		//Serial.println(ESP.getFreeHeap(), DEC);

		// save RGB LED status every one minute if required
		if(eepromUpdatePending == true) {

			DEBUG_PRINT("saveCapabilities pin ");DEBUG_PRINTLN(pin);
			saveCapabilities();
		}
	}
}

#endif
