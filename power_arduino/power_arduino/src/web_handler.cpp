#include <ESP8266WebServer.h>

extern ESP8266WebServer server;

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
    websock.send(
      '{' +
      '"iddisp":"'+document.getElementById('iddisp').value + '",' +
      '"ssidwifi":"'+document.getElementById('ssidwifi').value + '",' +
      '"passwifi":"'+document.getElementById('passwifi').value + '",' +
      '"directmp":"'+document.getElementById('directmp').value + '",' +
      '"frec":"'+document.getElementById('frec').value + '",' +
      '"port":"'+document.getElementById('port').value + '",' +
      '"direcip0":"'+document.getElementById('direcip0').value + '",' +
      '"direcip1":"'+document.getElementById('direcip1').value + '",' +
      '"direcip2":"'+document.getElementById('direcip2').value + '",' +
      '"direcip3":"'+document.getElementById('direcip3').value + '",' +
      '"tip":"'+document.getElementById('tip').value + '",' +
      '"protoc":"'+document.getElementById('protoc').value + '"' +
      '}'
    );
  }
}
</script>
</head>
<body onload="javascript:start();">
<h1>Configuraci&oacute;n del medidor de potencia</h1>
<div><b>ID dispositivo</b></div>
<input id="iddisp" type="text" size="63" maxlength="63" value="visualizee.greencpd.b039.rack.r0002.server.frodo" name="iddispositivo">
<div><b>Nombre del WiFi</b></div>
<input id="ssidwifi" type="text" size="31" maxlength="31" value="GreenLSI" name="nombrewifi">
<div><b>Contrase&ntilde;a del WiFi</b></div>
<input id="passwifi" type="text" size="31" maxlength="31" value="" name="passwordwwifi">
<div><b>Direcci&oacute;n tiempo</b></div>
<input id="directmp" type="text" size="31" maxlength="31" value="time.nist.gov" name="directiempo">
<div><b>Periodo de medida (ms)</b></div>
<input id="frec" type="text" size="15" maxlength="15" value="5000" name="frecuencia">
<div><b>Puerto</b></div>
<input id="port" type="text" size="5" maxlength="5" value="2003" name="puerto">
<div><b>Direcci&oacute;n IP</b></div>
<input id="direcip0" type="text" size="3" maxlength="3" value="138">
<input id="direcip1" type="text" size="3" maxlength="3" value="4">
<input id="direcip2" type="text" size="3" maxlength="3" value="9">
<input id="direcip3" type="text" size="3" maxlength="3" value="56">
<div><b>Tipo</b></div>
<input id="tip" type="text" size="15" value="power" maxlength="15" name="tipo">
<div><b>Protocolo</b></div>
<input id="protoc" type="text" size="15" maxlength="15" value="MQTT/UDP" name="protocolo">
<button id="enviar"  type="button" onclick="buttonclick(this);">Enviar informaci&oacute;n</button>
</body>
</html>
)rawliteral";

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
