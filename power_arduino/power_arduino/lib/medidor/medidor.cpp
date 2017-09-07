/* Librería para la estructura de datos del SonoffPOW. */

/* Escrita por Oscar Vicente Mediavilla */

#include <EEPROM.h>
#include "medidor.h"

medidor::medidor() {}


void
medidor::update() {
  EEPROM.begin(4096);

  EEPROM.get(MEDIDOR_ID, IdDispositivo);  //Dirección 0.
  EEPROM.get(MEDIDOR_WIFI, WiFi);          //Dirección 69.
  EEPROM.get(MEDIDOR_PASWORD, Pasword);       //Dirección 101.
  EEPROM.get(MEDIDOR_FREC, Frecuencia);
  EEPROM.get(MEDIDOR_DIRECCIONIP, DireccionIP);  //Dirección 133.
  EEPROM.get(MEDIDOR_PUERTO, Puerto);       //Dirección 137.
  EEPROM.get(MEDIDOR_DIRECCIONTIEMPO, DireccionTiempo);
}

void
medidor::print() {
  Serial.println("---------");
  Serial.println("medidor::print");
  Serial.print("iddisp: ");
  Serial.println(IdDispositivo);

  Serial.print("ssidwifi: ");
  Serial.println(WiFi);

  Serial.print("passwifi: ");
  Serial.println(Pasword);

  Serial.print("frec: ");
  Serial.println(Frecuencia);

  Serial.print("direcip: ");
  Serial.print(DireccionIP[0]);
  Serial.print(".");
  Serial.print(DireccionIP[1]);
  Serial.print(".");
  Serial.print(DireccionIP[2]);
  Serial.print(".");
  Serial.println(DireccionIP[3]);

  Serial.print("port: ");
  Serial.println(Puerto);

  Serial.print("directmp: ");
  Serial.println(DireccionTiempo);
}

void
medidor::print_eeprom() {
  int i, j;
  unsigned char val;

  Serial.println("---------");
  Serial.println("medidor::print_eeprom");

  for (j = 0; j < 16; j++) {
    Serial.printf("%02d: ", j);
    for (i = 0; i < 16; i++) {
      val = EEPROM.read(j*16+i);
      Serial.print(val, HEX);
      Serial.print(' ');
    }
    Serial.print("\r\n");
  }
}
