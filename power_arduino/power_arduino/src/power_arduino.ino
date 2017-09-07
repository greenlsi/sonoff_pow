#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include "fsmpp.h"
#include "medidor.h"
#include "power.h"
#include <TimeLib.h>

#include <WebSocketsServer.h>
//#include <ESP8266WiFiMulti.h>
#include <Hash.h>
//#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include "ArduinoJson.h"

#define rele 12
#define boton 0

medidor mimedidor; //Estructura medidor.

ESP8266PowerClass power_sonoff;

WiFiUDP Udp; //Variable para el socket

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

//MDNSResponder mdns;

//ESP8266WiFiMulti WiFiMulti;

void sendInfo(const char* sensor, float value);
void punto_acceso();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void power_setup();
void delay_until (int frec_fsm);
unsigned long tiempo1970();
unsigned long sendNTPpacket(IPAddress& address);
void handleRoot();
void handleNotFound();

/***** ESTADOS FSM *****/
enum estados {
  IDLE,
  MEASURE,
  CONFIG_WAIT,
  CONFIG,
  };
/**********************/


//char IdDisp[64] = "Medidor de Potencia 1";

char IdDisp[64] = "visualizee.greencpd.b039.rack.r0002.server.frodo";

//char ssid[32] = "WLAN_B3";
//char pass[32] = "XE091532476B3";

char ssid[32] = "DIE_JAS";
char pass[32] = "12345678";

unsigned int frecuenc = 5000;

//unsigned char IP[4] = {192,168,1,37}; //Casa.

//unsigned char IP[4] = {10,2,0,101};

unsigned char IP[4] = {138,4,9,56}; ///visualizee

char ntpServerName[32] = "time.nist.gov";

int PuertoRemoto = 2003;

//char ssid[32] = "Especial_MCHP";
//char pass[32] = "M15SEB304";

//unsigned char IP[4] = {192,168,1,192}; //Especial_MCHP


/************ PARA CONECTAR CON EL SERVIDOR NTP ************/
IPAddress timeServerIP; 		// time.nist.gov NTP server (fallback)
const int NTP_PACKET_SIZE= 48; 				// La hora son los primeros 48 bytes del mensaje
byte packetBuffer[NTP_PACKET_SIZE]; 			// buffer para los paquetes

/***************************************/

struct mifsm_user_t {
  unsigned int previousMillis;
  unsigned int tiempo_pulsador;
  unsigned int end_time;
  unsigned int time_retry;
  unsigned int time_measure;
  unsigned int time_sync;
};

struct mifsm_user_t mifsm_user;


/***** CONDICIONES DE TRANSICIÓN *****/

static int tiempo_retry (fsmpp* _this){
  //Serial.println("tiempo_retry");
  struct mifsm_user_t* user = (struct mifsm_user_t*)_this->userInfo;
  return (millis() >= user->time_retry && digitalRead(boton) == HIGH) ? 1 : 0;};

static int connected (fsmpp* _this){
  return (WiFi.status() == WL_CONNECTED) ? 1 : 0;};

static int dissconnected (fsmpp* _this){
  return (WiFi.status() == WL_DISCONNECTED) ? 1 : 0;};

static int pulsador (fsmpp* _this){
  //int tiempo_pulsador = 5000;
  struct mifsm_user_t* user = (struct mifsm_user_t*)_this->userInfo;
  //Serial.println(user->tiempo_pulsador);
  return (digitalRead(boton) == LOW && user->tiempo_pulsador < 5000) ? 1 : 0;};

static int pulsador_timeout (fsmpp* _this){
  struct mifsm_user_t* user = (struct mifsm_user_t*)_this->userInfo;
  return (digitalRead(boton) == LOW && user->tiempo_pulsador >= 5000) ? 1 : 0;};

static int sync_timeout (fsmpp* _this){
    struct mifsm_user_t* user = (struct mifsm_user_t*)_this->userInfo;
    return (now() > user->time_sync) ? 1 : 0;};

static int tiempo_medida (fsmpp* _this){
  //Serial.println("tiempo_medida");
  //Serial.println(digitalRead(boton));
  struct mifsm_user_t* user = (struct mifsm_user_t*)_this->userInfo;
  return (millis() >= user->time_measure && digitalRead(boton) == HIGH) ? 1 : 0;};

static int pulsador_out (fsmpp* _this){
  struct mifsm_user_t* user = (struct mifsm_user_t*)_this->userInfo;
  return (digitalRead(boton) == HIGH && user->tiempo_pulsador != 0) ? 1 : 0;};

static int client_connect (fsmpp* _this){
  return (WiFi.softAPgetStationNum() == 1) ? 1 : 0;};

static int client_disconect (fsmpp* _this){
  return (WiFi.softAPgetStationNum() == 0) ? 1 : 0;};

static int check_true (fsmpp* _this){
  return 1;};

/***** SALIDAS TRAS CAMBIO DE ESTADO *****/

static void connect (fsmpp* _this){
  struct mifsm_user_t* user = (struct mifsm_user_t*)_this->userInfo;
  user->time_retry = millis() + 10000;
  int PuertoLocal = 4210;
  WiFi.begin(mimedidor.WiFi, mimedidor.Pasword);
  Udp.begin(PuertoLocal);
  //Serial.println("IDLE");
};

static void send_power (fsmpp* _this){
  struct mifsm_user_t* user = (struct mifsm_user_t*)_this->userInfo;
  const char* tipo = "power";
  sendInfo(tipo, power_sonoff.getPower());
  user->time_measure = millis() + mimedidor.Frecuencia;
  Serial.println("MEASURE");
};

static void time_annotate (fsmpp* _this){
  int tiempo_pulsador = 5000;
  struct mifsm_user_t* user = (struct mifsm_user_t*)_this->userInfo;
  if (user->end_time == 0){
    user->end_time = millis() + tiempo_pulsador;
    user->previousMillis = millis();
  }
  else {
    user->tiempo_pulsador = millis() - user->previousMillis;
  }
  //Serial.println("time_annotate");
};

static void time_reset (fsmpp* _this){
  struct mifsm_user_t* user = (struct mifsm_user_t*)_this->userInfo;
  mimedidor.print();
  mimedidor.print_eeprom();
  user->tiempo_pulsador = 0;
  user->end_time = 0;
};

static void access_point (fsmpp* _this){
  struct mifsm_user_t* user = (struct mifsm_user_t*)_this->userInfo;
  user->tiempo_pulsador = 0;
  user->end_time = 0;
  Udp.stop();
  punto_acceso();

  server.on("/", handleRoot);//
  server.onNotFound(handleNotFound);//
  server.begin(); //

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("CONFIG_WAIT");
};

static void sync_time (fsmpp* _this){
  struct mifsm_user_t* user = (struct mifsm_user_t*)_this->userInfo;
  time_t tiempo;

  tiempo=tiempo1970();
  setTime(tiempo);

  user->time_sync = tiempo + 60;
  Serial.printf("Time sync: %ld\n", tiempo);
}

static void set_up (fsmpp* _this){ //Se puede poner también para connected
  struct mifsm_user_t* user = (struct mifsm_user_t*)_this->userInfo;

  user->time_measure = millis() + mimedidor.Frecuencia;
  mimedidor.update();
  int PuertoLocal = 4210;
  Udp.begin(PuertoLocal);
};

static void mensaje (fsmpp* _this){ //Se puede poner también para connected
  Serial.println("esperando");
};

static void ws_loop (fsmpp* _this){ //Se puede poner también para connected
  webSocket.loop();
  server.handleClient();
};

/*******************    TABLA DE TRANSICIONES    *******************/
fsmpp_trans_t trans[] = {
  { IDLE,         tiempo_retry,      IDLE,         connect },
  { IDLE,         pulsador,          IDLE,         time_annotate },
  { IDLE,         pulsador_timeout,  CONFIG_WAIT,  access_point },
  { IDLE,         pulsador_out,      IDLE,         time_reset },
  { IDLE,         connected,         MEASURE,      set_up },
  { MEASURE,      sync_timeout,      MEASURE,      sync_time },
  { MEASURE,      tiempo_medida,     MEASURE,      send_power },
  { MEASURE,      pulsador,          MEASURE,      time_annotate },
  { MEASURE,      pulsador_out,      MEASURE,      time_reset },
  { MEASURE,      dissconnected,     IDLE,         NULL },
  { MEASURE,      pulsador_timeout,  CONFIG_WAIT,  access_point },
  { CONFIG_WAIT,  client_connect,    CONFIG,       mensaje},
  //{ CONFIG,       data,              CONFIG,       process_data },
  { CONFIG,       client_disconect,  MEASURE,      set_up },
  { CONFIG,       check_true,        CONFIG,       ws_loop },
  { CONFIG,       pulsador,          CONFIG,       time_annotate },
  { CONFIG,       pulsador_out,      IDLE,         time_reset },
  {-1, NULL, -1, NULL },
};
/*******************************************************************/

fsmpp myfsm;


void setup() {

  EEPROM.begin(4096); //Inicialización de la memoria EEPROM, tamaño 4096 bytes.
  //EEPROM_load(); //Se cargan los parámetros en memoria.
  mimedidor.update(); //Se asocia la estructura a los parámetros de memoria.

  power_setup();

  Serial.begin(9600);

  myfsm.fsm_init(trans);
  memset(&mifsm_user, 0, sizeof(mifsm_user_t));
  myfsm.userInfo = &mifsm_user;
}

void loop() {

  myfsm.fsm_fire();
  delay_until(50);
}

void EEPROM_load(){
  int i;

  EEPROM.put(MEDIDOR_ID,IdDisp); //Dirección 0.
  EEPROM.put(MEDIDOR_WIFI, ssid); //Dirección 69.
  EEPROM.put(MEDIDOR_PASWORD, pass); //Dirección 101.
  for (i = 0; i < 4; i++) {
    EEPROM.write(MEDIDOR_DIRECCIONIP+i,IP[i]);   //Dirección 133-136.
  }
  EEPROM.put(MEDIDOR_PUERTO, PuertoRemoto); //Dirección 137.
  EEPROM.put(MEDIDOR_FREC, frecuenc);
  EEPROM.put(MEDIDOR_DIRECCIONTIEMPO, ntpServerName);

  EEPROM.commit(); //Este comando guarda lo escrito en la EEPROM.
}

void power_setup(){
  pinMode(boton,INPUT); //Se habilita el pin del botón como entrada.
  pinMode(rele,OUTPUT); //Se habilita el pin del relé como salida.
  digitalWrite(rele,HIGH); //Se activa el relé para activar el conmutador.

  power_sonoff.enableMeasurePower(); //Se habilita la medida.
  power_sonoff.startMeasure(); //Se comienza la medición.
}

void sendInfo(const char* sensor, float value) {

  char buffer[128];
  unsigned long tiempo=0;
  //tiempo=tiempo1970();
  tiempo=now();
  Serial.println(tiempo);
  Serial.println(now());

  sprintf(buffer, "%s.%s.1 %s %s", mimedidor.IdDispositivo, sensor,  String(value).c_str(), String(tiempo).c_str());
  Udp.beginPacket(mimedidor.DireccionIP, mimedidor.Puerto); //2003
  Udp.write(buffer);
  Udp.endPacket();
}


unsigned long tiempo1970() {
  //Obtiene una dirección del servidor.
  WiFi.hostByName(mimedidor.DireccionTiempo, timeServerIP);
  int cb=0;
  while (!cb){
    Serial.println("no packet yet");
    sendNTPpacket(timeServerIP); //Envía paquete NTP al servidor.
    delay(1000);//Espera respuesta.
     cb = Udp.parsePacket();
  }
  Udp.read(packetBuffer, NTP_PACKET_SIZE);

  //El tamaño del paquete es de 48 bytes y el tiempo empieza en el byte 40,
  //y ocupa 4 bytes.
  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  unsigned long secsSince1900 = highWord << 16 | lowWord;
  //Este es el tiempo desde el 1 de enero de 1900.
  const unsigned long seventyYears = 2208988800UL; //70 años
  unsigned long epoch = secsSince1900 - seventyYears;

  return epoch;
  }


unsigned long sendNTPpacket(IPAddress& address) {
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011;	  // LI, Version, Mode
	packetBuffer[1] = 0;	   // Stratum, or type of clock
	packetBuffer[2] = 6;	   // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12]	= 49;
	packetBuffer[13]	= 0x4E;
	packetBuffer[14]	= 49;
	packetBuffer[15]	= 52;

	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:

	Udp.beginPacket(address, 123); //NTP requests are to port 123
	Udp.write(packetBuffer,NTP_PACKET_SIZE);
	Udp.endPacket();
}

void delay_until (int frec_fsm) {
  int previous = millis();
  while(previous+frec_fsm>=millis()){}
}

void punto_acceso(){

  String MAC = WiFi.softAPmacAddress();
  String dirMAC = "ESP_"+MAC.substring(9,11)+MAC.substring(12,14)+MAC.substring(15,17);
  static char ssid_AP[11];
  static char password[9];

  WiFi.disconnect(false);

  strcpy(ssid_AP, dirMAC.c_str());
  strcpy(password, "password");

  //IPAddress IP_AP(192,168,1,1);
  //  IPAddress NMask(255,255,255,0);

  //WiFi.softAPConfig(IP_AP, IP_AP, NMask);

  Serial.println();
  Serial.print("Configuring access point...");
  //Wifi.begin(ssid_AP, password);
  if (! WiFi.softAP(ssid_AP, password)) {
    Serial.println("WiFi softAP failed");
    //return;
  }

  IPAddress myIP = WiFi.softAPIP();

  Serial.print("AP IP address: ");
  Serial.println(myIP);

  Serial.println();
  Serial.println();

  for(uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\r\n", t);
    Serial.flush();
    delay(1000);
  }

  //WiFiMulti.addAP(ssid_AP, password);

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid_AP);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  //if (mdns.begin(ssid_AP, WiFi.softAPIP())) {
/*
  if (mdns.begin(ssid_AP)) {
    Serial.println("MDNS responder started");
    mdns.addService("http", "tcp", 80);
    mdns.addService("ws", "tcp", 81);
  }
  else {
    Serial.println("MDNS.begin failed");
  }
*/
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);

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

      EEPROM.begin(4096);

      Serial.print("iddisp: ");
      strcpy(IdDisp, config["iddisp"]);
      Serial.println(IdDisp);

      Serial.print("ssidwifi: ");
      strcpy(ssid, config["ssidwifi"]);
      Serial.println(ssid);

      Serial.print("passwifi: ");
      strcpy(pass, config["passwifi"]);
      Serial.println(pass);

      Serial.print("directmp: ");
      strcpy(ntpServerName, config["directmp"]);
      Serial.println(ntpServerName);

      Serial.print("frec: ");
      frecuenc = atoi(config["frec"]);
      Serial.println(frecuenc);

      Serial.print("port: ");
      PuertoRemoto = atoi(config["port"]);
      Serial.println(PuertoRemoto);

      Serial.print("direcip: ");
      IP[0] = atoi(config["direcip0"]);
      IP[1] = atoi(config["direcip1"]);
      IP[2] = atoi(config["direcip2"]);
      IP[3] = atoi(config["direcip3"]);
      Serial.print(IP[0]);
      Serial.print(".");
      Serial.print(IP[1]);
      Serial.print(".");
      Serial.print(IP[2]);
      Serial.print(".");
      Serial.println(IP[3]);

      Serial.print("tip: ");
      const char* tip = config["tip"];
      Serial.println(tip);

      Serial.print("protoc: ");
      const char* protoc = config["protoc"];
      Serial.println(protoc);

      EEPROM_load(); //Este comando guarda lo escrito en la EEPROM.

      webSocket.broadcastTXT(payload, length);

      break;
    }
    /*case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\r\n", num, length);
      hexdump(payload, length);

      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;*/
    default:
      Serial.printf("Invalid WStype [%d]\r\n", type);
      break;
  }
}
