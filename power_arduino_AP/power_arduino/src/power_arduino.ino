#include <Arduino.h>
#include <ESP8266WiFi.h>
//#include <WiFiClient.h>
#include <WebSocketsServer.h>
#include <ESP8266WiFiMulti.h>
#include <Hash.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <sys/time.h>
#include "ArduinoJson.h"

/********** EL ID DE LA WiFi SE GENERA A PARTIR DE LA MAC **********/
String MAC = WiFi.macAddress();
String dirMAC = "ESP_"+MAC.substring(9,11)+MAC.substring(12,14)+MAC.substring(15,17);
const char *ssid = dirMAC.c_str();

const char *password = "password";

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
/*************************************************/

MDNSResponder mdns;
struct timeval next_activation;

static void writeLED(bool);

//ESP8266WiFiMulti WiFiMulti;

/**********  HTML PARA EL SERVIDOR WEB  **********/

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
<meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
<title>ESP8266 WebSocket Demo</title>
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
  };
}
function buttonclick(e) {
  if (e.id === 'enviar') {
    websock.send('iddisp:' + document.getElementById('iddisp').value + ';' +
    'ssidwifi:' + document.getElementById('ssidwifi').value + ';' +
    'passwifi:' + document.getElementById('passwifi').value + ';' +
    'directmp:' + document.getElementById('directmp').value + ';' +
    'frec:' + document.getElementById('frec').value + ';' +
    'port:' + document.getElementById('port').value + ';' +
    'direcip:' + document.getElementById('direcip').value + ';' +
    'tip:' + document.getElementById('tip').value + ';' +
    'protoc:' + document.getElementById('protoc').value + ';');
  }
}
</script>
</head>
<body onload="javascript:start();">
<h1>Configuraci&oacute;n del medidor de potencia</h1>
<div id="ledstatus"><b>LED</b></div>
<button id="ledon"  type="button" onclick="buttonclick(this);">On</button>
<button id="ledoff" type="button" onclick="buttonclick(this);">Off</button>
<div><b>ID dispositivo</b></div>
<input id="iddisp" type="text" size="15" maxlength="30" value="Nombre del medidor" name="iddispositivo">
<div><b>Nombre del WiFi</b></div>
<input id="ssidwifi" type="text" size="15" maxlength="30" value="Nombre WiFi" name="nombrewifi">
<div><b>Contrase&ntilde;a del WiFi</b></div>
<input id="passwifi" type="text" size="15" maxlength="30" value="Pasw WiFi" name="passwordwwifi">
<div><b>Direcci&oacute;n tiempo</b></div>
<input id="directmp" type="text" size="15" maxlength="30" value="Pasw WiFi" name="directiempo">
<div><b>Frecuencia de medida</b></div>
<input id="frec" type="text" size="15" maxlength="30" value="frecuencia" name="frecuencia">
<div><b>Puerto</b></div>
<input id="port" type="text" size="15" maxlength="30" value="xxxx" name="puerto">
<div><b>Direcci&oacute;n IP</b></div>
<input id="direcip" type="text" size="15" maxlength="30" value="xxx.xxx.xxx.xxx" name="nombre">
<div><b>Tipo</b></div>
<input id="tip" type="text" size="15" maxlength="30" name="tipo">
<div><b>Protocolo</b></div>
<input id="protoc" type="text" size="15" maxlength="30" value="MQTT/UDP" name="protocolo">
<button id="enviar"  type="button" onclick="buttonclick(this);">Enviar informaci&oacute;n</button>
</body>
</html>
)rawliteral";

/*************************************************/

// GPIO#0 is for Adafruit ESP8266 HUZZAH board. Your board LED might be on 13.
const int LEDPIN = 13;
// Current LED status
bool LEDStatus;

// Commands sent through Web Socket
const char LEDON[] = "ledon";
const char LEDOFF[] = "ledoff";



/********** ENVÍO DEL SERVIDOR **********/
void handleRoot() {

  server.send(200, "text/html", INDEX_HTML);
  //server.send(200, "text/html", "<h1>You are connected</h1>");
  }
/*************************************************/

/********** SI NO SE ESTABLECE LA CONEXIÓN **********/

void handleNotFound() {

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
  server.send(404, "text/plain", message);
  }
/*************************************************/

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
          webSocket.sendTXT(num, LEDON, strlen(LEDON));
        }
        else {
          webSocket.sendTXT(num, LEDOFF, strlen(LEDOFF));
        }
      }
      break;
    case WStype_TEXT: {
      Serial.printf("[%u] get Text: %s\r\n", num, payload);
      StaticJsonBuffer<1000> jsonBuffer;

      JsonObject& config = jsonBuffer.parseObject(payload);

      if (!config.success()) {
        Serial.println("Parse error");
        break;
      }

      Serial.print("iddisp: ");
      const char* iddisp = config["iddisp"];
      Serial.println(iddisp);

      if (strcmp(LEDON, (const char *)payload) == 0) {
        writeLED(true);
      }
      else if (strcmp(LEDOFF, (const char *)payload) == 0) {
        writeLED(false);
      }
      else {
        Serial.println("Unknown command");
      }
      // send data to all connected clients
      webSocket.broadcastTXT(payload, length);
      break;
    }
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

static void writeLED(bool LEDon)
{
  LEDStatus = LEDon;
  // Note inverted logic for Adafruit HUZZAH board
  if (LEDon) {
    digitalWrite(LEDPIN, 0);
  }
  else {
    digitalWrite(LEDPIN, 1);
  }
}


void setup() {

delay(5000);

gettimeofday(&next_activation, NULL);

/***** CONFIGURACIÓN DEL PUNTO DE ACCESO *****/

  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");

  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();

  Serial.print("AP IP address: ");
  Serial.println(myIP);

  /*
  server.on("/", handleRoot);

  server.begin();

  Serial.println("HTTP server started");*/

  pinMode(LEDPIN, OUTPUT);
  writeLED(false);

  //Serial.begin(115200);

  //Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  for(uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\r\n", t);
    Serial.flush();
    delay(1000);
  }

  //WiFiMulti.addAP(ssid, password);

  //while(WiFiMulti.run() != WL_CONNECTED) {
  //  Serial.print(".");
  //  delay(100);
  //}

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (mdns.begin("espWebSock", WiFi.localIP())) {
    Serial.println("MDNS responder started");
    mdns.addService("http", "tcp", 80);
    mdns.addService("ws", "tcp", 81);
  }
  else {
    Serial.println("MDNS.begin failed");
  }
  Serial.print("Connect to http://espWebSock.local or http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  }

void loop() {

  webSocket.loop();
  server.handleClient();

  }
