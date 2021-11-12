#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ESPmDNS.h>

//#define WIFI_AP_MODE  true  //Uncomment select WIFI_AP mode
#define WIFI_STA_MODE true   //Uncomment select WIFI_STA mode

#define GREEN_LED  3
#define RELAY_PIN 9
bool LEDStatus;
// Commands sent through Web Socket
const char RelayON[] = "RelayON";
const char RelayOFF[] = "RelayOFF";

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
<title>T-01C3 WebSocket Demo</title>
<style>
"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }"
</style>
<script>
var websock;
function start() {
  websock = new WebSocket('ws://' + window.location.hostname + ':81/');
  websock.onopen = function(evt) { console.log('websock open'); };
  websock.onclose = function(evt) { console.log('websock close'); };
  websock.onerror = function(evt) { console.log(evt); };
  websock.onmessage = function(evt) {
    console.log(evt);
    var e = document.getElementById('ledstatus');
    if (evt.data === 'RelayON') {
      e.style.color = 'red';
    }
    else if (evt.data === 'RelayOFF') {
      e.style.color = 'black';
    }
    else {
      console.log('unknown event');
    }
  };
}
function buttonclick(e) {
  websock.send(e.id);
}
</script>
</head>
<body onload="javascript:start();">
<h1>T-01C3 WebSocket Demo</h1>
<div id="ledstatus"><b>Relay</b></div>
<button id="RelayON"  type="button" style="width:100px;height:60px" background-color="green" onclick="buttonclick(this);">On</button> 
<button id="RelayOFF" type="button" style="width:100px;height:60px" background-color="red"  onclick="buttonclick(this);">Off</button>
</body>
</html>
)rawliteral";


#ifdef WIFI_STA_MODE
const char* ssid = "SSID"; // SSID
const char* password = ""; // Password
#elif WIFI_AP_MODE

const char* ssid = "T-01C3"; // SSID
const char* password = ""; // Password
#endif

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

//root page can be accessed only if authentification is ok
void handleRoot() {

  server.send_P(200, "text/html", INDEX_HTML);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void hexdump(const void *mem, uint32_t len, uint8_t cols = 16) {
	const uint8_t* src = (const uint8_t*) mem;
	Serial1.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);
	for(uint32_t i = 0; i < len; i++) {
		if(i % cols == 0) {
			Serial1.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
		}
		Serial1.printf("%02X ", *src);
		src++;
	}
	Serial1.printf("\n");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        // Send the current LED status
        if (LEDStatus) {
          webSocket.sendTXT(num, RelayON, strlen(RelayON));
        }
        else {
          webSocket.sendTXT(num, RelayOFF, strlen(RelayOFF));
        }
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\r\n", num, payload);

      if (strcmp(RelayON, (const char *)payload) == 0) {
        writeRELAY(true);
      }
      else if (strcmp(RelayOFF, (const char *)payload) == 0) {
        writeRELAY(false);
      }
      else {
        Serial.println("Unknown command");
      }
      // send data to all connected clients
      webSocket.broadcastTXT(payload, length);
      break;
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\r\n", num, length);
      hexdump(payload, length);

      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;
    default:
      Serial.printf("Invalid WStype [%d]\r\n", type);
      break;
  }
}

static void writeRELAY(bool on)
{
  LEDStatus = on;
  // Note inverted logic for Adafruit HUZZAH board
  if (on) {
    digitalWrite(RELAY_PIN, HIGH);
  }
  else {
    digitalWrite(RELAY_PIN, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(GREEN_LED,OUTPUT);
  pinMode(RELAY_PIN,OUTPUT);
  digitalWrite(GREEN_LED,HIGH);
  digitalWrite(RELAY_PIN,LOW);
  delay(1000);

#ifdef WIFI_STA_MODE
  /* Connect WiFi */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

#elif WIFI_AP_MODE
 WiFi.softAP(ssid, password);

  if (MDNS.begin("T-01C3")) {
    Serial.println("MDNS responder started");
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
    }
      else {
    Serial.println("MDNS.begin failed");
  }
    Serial.println("mDNS responder started");
  Serial.print("Connect to http://T-01C3.local");

#endif
  

  server.on("/", handleRoot);

  server.onNotFound(handleNotFound);
  //here the list of headers to be recorded
  const char * headerkeys[] = {"User-Agent", "Cookie"} ;
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
  //ask server to track these headers
  server.collectHeaders(headerkeys, headerkeyssize);
  server.begin();
  Serial.println("HTTP server started");

  
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  
  webSocket.loop();
  server.handleClient();

}
