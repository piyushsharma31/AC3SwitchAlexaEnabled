#include "AlexaDevice.h"
#include "CallbackFunction.h"

//<<constructor>>
AlexaDevice::AlexaDevice(){
	Serial.println("default constructor called");
}

//AlexaDevice::AlexaDevice(String alexaInvokeName,unsigned int port){
AlexaDevice::AlexaDevice(String alexaInvokeName, unsigned int _port, CallbackFunction oncb, CallbackFunction offcb, CallbackFunction statuscb) {

  device_name = alexaInvokeName;
  port = _port;
  onCallback = oncb;
  offCallback = offcb;
  statusCallback = statuscb;

	uint32_t chipId = ESP.getChipId();
	char uuid[64];
	sprintf_P(uuid, PSTR("38323636-4558-4dda-9188-cda0e6%02x%02x%02x"),
	(uint16_t) ((chipId >> 16) & 0xff),
	(uint16_t) ((chipId >>  8) & 0xff),
	(uint16_t)   chipId        & 0xff);
	
	serial = String(uuid);
	persistent_uuid = "Socket-1_0-" + serial+"-"+ String(port);
	
	startWebServer();
Serial.print("AlexaDevice::AlexaDevice port ");Serial.println(port);
Serial.print("AlexaDevice::AlexaDevice persistent_uuid ");Serial.println(persistent_uuid);
}

//<<destructor>>
AlexaDevice::~AlexaDevice(){/*nothing to destruct*/}

void AlexaDevice::serverLoop(){
	if (server != NULL) {
		server->handleClient();
		delay(1);
	}
}

void AlexaDevice::startWebServer(){
	server = new ESP8266WebServer(port);

	server->on("/", [&]() {
		handleRoot();
	});


	server->on("/setup.xml", [&]() {
		handleSetupXml();
	});

	server->on("/upnp/control/basicevent1", [&]() {
		handleUpnpControl();
	});

	server->on("/eventservice.xml", [&]() {
		handleEventservice();
	});

	//server->onNotFound(handleNotFound);
	server->begin();

	Serial.println("WebServer started on port: ");
	Serial.println(port);
}

void AlexaDevice::handleEventservice(){
	Serial.println(" ########## Responding to eventservice.xml ... ########\n");

	String eventservice_xml = "<scpd xmlns=\"urn:Belkin:service-1-0\">"
	"<actionList>"
	"<action>"
	"<name>SetBinaryState</name>"
	"<argumentList>"
	"<argument>"
	"<retval/>"
	"<name>BinaryState</name>"
	"<relatedStateVariable>BinaryState</relatedStateVariable>"
	"<direction>in</direction>"
	"</argument>"
	"</argumentList>"
	"</action>"
	"<action>"
	"<name>GetBinaryState</name>"
	"<argumentList>"
	"<argument>"
	"<retval/>"
	"<name>BinaryState</name>"
	"<relatedStateVariable>BinaryState</relatedStateVariable>"
	"<direction>out</direction>"
	"</argument>"
	"</argumentList>"
	"</action>"
	"</actionList>"
	"<serviceStateTable>"
	"<stateVariable sendEvents=\"yes\">"
	"<name>BinaryState</name>"
	"<dataType>Boolean</dataType>"
	"<defaultValue>0</defaultValue>"
	"</stateVariable>"
	"<stateVariable sendEvents=\"yes\">"
	"<name>level</name>"
	"<dataType>string</dataType>"
	"<defaultValue>0</defaultValue>"
	"</stateVariable>"
	"</serviceStateTable>"
	"</scpd>\r\n"
	"\r\n";
	
	server->send(200, "text/plain", eventservice_xml.c_str());
}

void AlexaDevice::handleUpnpControl(){
	Serial.println("########## Responding to  /upnp/control/basicevent1 ... ##########");      

	//for (int x=0; x <= HTTP.args(); x++) {
	//  Serial.println(HTTP.arg(x));
	//}

	String request = server->arg(0);      
	Serial.print("request:");
	Serial.println(request);


	if(request.indexOf("SetBinaryState") >= 0) {
		if(request.indexOf("<BinaryState>1</BinaryState>") >= 0) {
			Serial.println("Got Turn on request");
			switchStatus = onCallback();

			sendRelayState();
		}

		if(request.indexOf("<BinaryState>0</BinaryState>") >= 0) {
			Serial.println("Got Turn off request");
			switchStatus = offCallback();
			
			sendRelayState();
		}
	}

	if(request.indexOf("GetBinaryState") >= 0) {
		Serial.println("Got binary state request");
		switchStatus = statusCallback();

		sendRelayState();
	}

	server->send(200, "text/plain", "");
}

void AlexaDevice::handleRoot(){
	server->send(200, "text/plain", "You should tell Alexa to discover devices");
}

void AlexaDevice::handleSetupXml(){
	Serial.println(" ########## Responding to setup.xml ... ########\n");

	IPAddress localIP = WiFi.localIP();
	char s[16];
	sprintf(s, "%d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);


	String setup_xml = "<?xml version=\"1.0\"?>"
	"<root>"
	"<device>"
	"<deviceType>urn:Belkin:device:controllee:1</deviceType>"
	"<friendlyName>"+ device_name +"</friendlyName>"
	"<manufacturer>Belkin International Inc.</manufacturer>"
	"<modelName>Socket</modelName>"
	"<modelNumber>3.1415</modelNumber>"
	"<modelDescription>Belkin Plugin Socket 1.0</modelDescription>\r\n"
	"<UDN>uuid:"+ persistent_uuid +"</UDN>"
	"<serialNumber>221517K0101769</serialNumber>"
	"<binaryState>0</binaryState>"
	"<serviceList>"
	"<service>"
	"<serviceType>urn:Belkin:service:basicevent:1</serviceType>"
	"<serviceId>urn:Belkin:serviceId:basicevent1</serviceId>"
	"<controlURL>/upnp/control/basicevent1</controlURL>"
	"<eventSubURL>/upnp/event/basicevent1</eventSubURL>"
	"<SCPDURL>/eventservice.xml</SCPDURL>"
	"</service>"
	"</serviceList>" 
	"</device>"
	"</root>\r\n"
	"\r\n";

	
	server->send(200, "text/xml", setup_xml.c_str());
	
	Serial.print("Sending :");
	Serial.println(setup_xml);
}

String AlexaDevice::getAlexaInvokeName() {
	return device_name;
}

void AlexaDevice::sendRelayState() {
	String body = 
	"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body>\r\n"
	"<u:GetBinaryStateResponse xmlns:u=\"urn:Belkin:service:basicevent:1\">\r\n"
	"<BinaryState>";
	
	//body += (switchStatus ? "1" : "0");
	body += (switchStatus);

	body += "</BinaryState>\r\n"
	"</u:GetBinaryStateResponse>\r\n"
	"</s:Body> </s:Envelope>\r\n";

	server->send(200, "text/xml", body.c_str());

	Serial.print("Sending :");
	Serial.println(body);
}

void AlexaDevice::respondToSearch(IPAddress& senderIP, unsigned int senderPort) {
	Serial.println("");
	Serial.print("Sending response to ");
	Serial.print(senderIP);
	Serial.print(" Port : ");
	Serial.println(senderPort);

	IPAddress localIP = WiFi.localIP();
	char s[16];
	sprintf(s, "%d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);

	String response = 
	"HTTP/1.1 200 OK\r\n"
	"CACHE-CONTROL: max-age=86400\r\n"
	"DATE: Sat, 26 Nov 2016 04:56:29 GMT\r\n"
	"EXT:\r\n"
	"LOCATION: http://" + String(s) + ":" + String(port) + "/setup.xml\r\n"
	"OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
	"01-NLS: b9200ebb-736d-4b93-bf03-835149d13983\r\n"
	"SERVER: Unspecified, UPnP/1.0, Unspecified\r\n"
	"ST: urn:Belkin:device:**\r\n"
	"USN: uuid:" + persistent_uuid + "::urn:Belkin:device:**\r\n"
	"X-User-Agent: redsonic\r\n\r\n";

	UDP.beginPacket(senderIP, senderPort);
	UDP.write(response.c_str());
	UDP.endPacket();
	/* add yield to fix UDP sending response. For more informations : https://www.tabsoverspaces.com/233359-udp-packets-not-sent-from-esp-8266-solved */
	yield();      

	Serial.println("Response sent !");
}
