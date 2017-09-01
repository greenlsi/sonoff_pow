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
