/*
* RSA webSocket esp8266
* © RSA 2017
*
*/
#define LED 13
#define LED_RED 15
#define LED_GREEN 13
#define LED_BLUE 12

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <FS.h>

const char *ssid = "*************";
const char *password = "****************";

ESP8266WebServer webServer (80);
WebSocketsServer webSocket = WebSocketsServer(81);

unsigned long prevTime = 0;
unsigned long p1Time = 125;
bool thereIsConnection = false;
uint8_t *value;


void setup() {
	pinMode(LED, OUTPUT);
	pinMode(LED_RED, OUTPUT);
	pinMode(LED_GREEN, OUTPUT);
	pinMode(LED_BLUE, OUTPUT);
	pinMode(A0, INPUT);
	
	digitalWrite(LED, 0);
	
	Serial.begin(115200);
	WiFi.begin(ssid, password);
	Serial.println("");
	while(WiFi.status() != WL_CONNECTED){
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	webSocket.begin();
	webSocket.onEvent(webSocketEvent);
	
	if(SPIFFS.begin()){
		
	}else{
		Serial.println("SPIFFS.begin() error");
	}
	
	//webServer.serveStatic("/", SPIFFS, "/index.htm");
	webServer.onNotFound([](){
		if(!handleFileRead(webServer.uri()))
			webServer.send(404, "text/plain", "404: Not Found");
	});
	webServer.begin();
	Serial.println("WebServer started");
}

void loop() {
	webServer.handleClient();
	webSocket.loop();
	unsigned long curTime = millis();
	if(curTime - prevTime > p1Time){
		prevTime = curTime;
		tick();
	}
}

void listRoot(){
	Serial.println("SPIFFS root:");
	Dir dir = SPIFFS.openDir("/");
	while(dir.next()){
		Serial.print(dir.fileName());
		Serial.print("(");
		Serial.print(dir.fileSize());
		Serial.println(")");
	}
}

void tick(){
	if(thereIsConnection){
		uint16_t value = analogRead(A0);
		uint8_t val[2];
		val[1] = value & 255;
		val[0] = value>>8 & 255;
		webSocket.broadcastBIN(val, 2);
		Serial.print("!");
	}else{
		Serial.print(".");
	}
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
	switch(type){
		case WStype_DISCONNECTED:
		{
			Serial.printf("[%u] Disconnected!\n", num);
			thereIsConnection = false;
		}
			break;
		case WStype_CONNECTED:
		{
			IPAddress ip = webSocket.remoteIP(num);
			Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
			webSocket.sendTXT(num, "Connected");
			thereIsConnection = true;
		}
			break;
		case WStype_TEXT:
		{
			Serial.printf("[%u] get Text: %s\n", num, payload);
			if(payload[0] == '#') {
				uint32_t rgb = (uint32_t) strtol((const char *) &payload[1], NULL, 16);
				analogWrite(LED_RED,    ((rgb >> 16) & 0xFF));
				analogWrite(LED_GREEN,  ((rgb >> 8) & 0xFF));
				analogWrite(LED_BLUE,   ((rgb >> 0) & 0xFF));
			}
		}
			break;
		case WStype_BIN:
		{
			Serial.printf("[%u] get binary length: %u\n", num, length);
			hexdump(payload, length);
		}
			break;
	}
}

bool handleFileRead(String path){
	Serial.println("handleFileRead: " + path);
	if(path.endsWith("/")){path += "index.htm";}
	String contentType = getContentType(path);
	if(SPIFFS.exists(path)){
		File file = SPIFFS.open(path, "r");
		size_t sent = webServer.streamFile(file, contentType);
		file.close();
		return true;
	}
	Serial.println("\tFile Not Found");
	return false;
}



String getContentType(String filename){
	if(filename.endsWith(".htm")) return "text/html";
	else if(filename.endsWith(".html")) return "text/html";
	else if(filename.endsWith(".css")) return "text/css";
	else if(filename.endsWith(".js")) return "application/javascript";
	else if(filename.endsWith(".png")) return "image/png";
	else if(filename.endsWith(".gif")) return "image/gif";
	else if(filename.endsWith(".jpg")) return "image/jpeg";
	else if(filename.endsWith(".ico")) return "image/x-icon";
	else if(filename.endsWith(".xml")) return "text/xml";
	else if(filename.endsWith(".pdf")) return "application/x-pdf";
	else if(filename.endsWith(".zip")) return "application/x-zip";
	else if(filename.endsWith(".gz")) return "application/x-gzip";
	return "text/plain";
}
