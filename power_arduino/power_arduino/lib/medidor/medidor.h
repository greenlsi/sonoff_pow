/* Librería para la estructura de datos del SonoffPOW. */

/* Escrita por Oscar Vicente Mediavilla */

#ifndef MEDIDOR_H
#define MEDIDOR_H

#define MEDIDOR_ID  0
#define MEDIDOR_ID_LEN  64

#define MEDIDOR_WIFI  (MEDIDOR_ID+MEDIDOR_ID_LEN)
#define MEDIDOR_WIFI_LEN  32

#define MEDIDOR_PASWORD  (MEDIDOR_WIFI+MEDIDOR_WIFI_LEN)
#define MEDIDOR_PASWORD_LEN  32

#define MEDIDOR_DIRECCIONTIEMPO  (MEDIDOR_PASWORD+MEDIDOR_PASWORD_LEN)
#define MEDIDOR_DIRECCIONTIEMPO_LEN  32

#define MEDIDOR_FREC  (MEDIDOR_DIRECCIONTIEMPO+MEDIDOR_DIRECCIONTIEMPO_LEN)
#define MEDIDOR_FREC_LEN  sizeof(int)

#define MEDIDOR_PUERTO  (MEDIDOR_FREC+MEDIDOR_FREC_LEN)
#define MEDIDOR_PUERTO_LEN  sizeof(int)

#define MEDIDOR_DIRECCIONIP  (MEDIDOR_PUERTO+MEDIDOR_PUERTO_LEN)
#define MEDIDOR_DIRECCIONIP_LEN  4

#define MEDIDOR_TIPO  (MEDIDOR_DIRECCIONIP+MEDIDOR_DIRECCIONIP_LEN)
#define MEDIDOR_TIPO_LEN  sizeof(unsigned char)

#define MEDIDOR_PROTOCOLO  (MEDIDOR_TIPO+MEDIDOR_TIPO_LEN)
#define MEDIDOR_PROTOCOLO_LEN  sizeof(unsigned char)



#include <Arduino.h>

class medidor {	//Definición de la clase.

	public:
		medidor();	//Constructor.
		void update();

		char          IdDispositivo[MEDIDOR_ID_LEN]; //[64]
		char          WiFi[MEDIDOR_WIFI_LEN]; //[32]
		char          Pasword[MEDIDOR_PASWORD_LEN]; // [32]
		char          DireccionTiempo[MEDIDOR_DIRECCIONTIEMPO_LEN]; //[32]
		int           Frecuencia; //[4]
		int           Puerto; //[4]
		unsigned char DireccionIP[MEDIDOR_DIRECCIONIP_LEN]; //[4]
		unsigned char Tipo; //[1]
		unsigned char Protocolo; //[1]

	private: //No hace falta nada.

};

#endif
