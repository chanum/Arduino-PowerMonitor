//------------------------------------------------------------------------------
//- File         : PowerMonitor.ino
//- Company      : Kosten
//- Author       : Marcelo Alejandro Pouso
//- Address      : <mpouso@emtech.com.ar>
//- Created on   : 12:44:10-26/05/2019
//- Version      : 0.10
//- Description  : ...
//
//- Modification History:
//- Date        By    Version    Change Description
//------------------------------------------------------------------------------
//- 26/05/2019  MAP      0.10    Original
//------------------------------------------------------------------------------
#include "SIM900.h"
#include <SoftwareSerial.h>
#include "inetGSM.h"
#include <DHT11.h>

//----------------------------------------------------------------------------------
// Definiciones y variables globales
//----------------------------------------------------------------------------------

//---------
const String  THINGSPEAK_KEY = "IX1M46AF2ZWGIQYJ";

//---------
//SIM900
// Pin GSM TX 2
// Pin GSM RX 3
InetGSM inet;
char msg[50];
int numdata;
char inSerial[50];
boolean simStarted=false;

//---------
// DHT11
int pinDHT11=5;
DHT11 dht11(pinDHT11);


//----------------------------------------------------------------------------------
// Programa
//----------------------------------------------------------------------------------
/*
 * setup(): Configuracion inicial
 */
void setup()
{
  // Serial connection.
  Serial.begin(9600);
  Serial.println("---POWER MONITOR Init---");
  
  // Referemcia para sensor SCT013 30A/1V
  analogReference(INTERNAL);

  //Start configuration of shield with baudrate.
  //For http uses is raccomanded to use 4800 or slower.
  if (gsm.begin(2400)) {
    Serial.println("\nstatus=READY");
    simStarted=true;
  } else {
    Serial.println("\nstatus=IDLE");
  }
     
};


/*
 * loop(): Bucle principal
 */
void loop() {
  int err;
  float randNum, temp, hum, current, power;

  // Lectura sensor DHT11 ------------------
  // Si devuelve 0 es que ha leido bien
  if((err = dht11.read(hum, temp)) == 0) {
    Serial.print("Temperatura: ");
    Serial.print(temp);
    Serial.print(", Humedad: ");
    Serial.print(hum);
    Serial.println();
  }
  else {
    Serial.println();
    Serial.print("Error Num :");
    Serial.print(err);
    Serial.println();
  }

  // Lectura sensor SCT013 30A/1V ---------
  // Corriente eficaz - Irms
  current = getCurrent(); 
  // P = I*V (Watts)
  power = current*220.0;
  Serial.print("Irms: ");
  Serial.print(current);
  Serial.print(", Potencia: ");
  Serial.println(power);  
  
  // Publico Datos ------------------------
  // Si el moudulo sim900 esta inicializado correctamente
  // y no hay errores, envio datos a la nube
  if(simStarted && err==0) {
    // Random Channel 1
    randNum = random(5);
    sendData(THINGSPEAK_KEY, 1, randNum);

    // Temepratura Channel 2
    sendData(THINGSPEAK_KEY, 2, temp);

    // Humedad Channel 3
    sendData(THINGSPEAK_KEY, 3, hum);

    // Corriente Channel 4
    sendData(THINGSPEAK_KEY, 4, current);

    // Potencia Channel 5
    sendData(THINGSPEAK_KEY, 5, power);
  }
  delay(3000);
};


//----------------------------------------------------------------------------------
// Helpers
//----------------------------------------------------------------------------------

/*
 * getCurrent():
 */
float getCurrent() {
  float voltajeSensor;
  float corriente=0;
  float Sumatoria=0;
  long tiempo=millis();
  int N=0;
  while(millis()-tiempo<500)//Duración 0.5 segundos(Aprox. 30 ciclos de 60Hz)
  { 
    voltajeSensor = analogRead(A0) * (1.1 / 1023.0);////voltaje del sensor
    corriente=voltajeSensor*30.0; //corriente=VoltajeSensor*(30A/1V)
    Sumatoria=Sumatoria+sq(corriente);//Sumatoria de Cuadrados
    N=N+1;
    delay(1);
  }
  Sumatoria=Sumatoria*2;//Para compensar los cuadrados de los semiciclos negativos.
  corriente=sqrt((Sumatoria)/N); //ecuación del RMS
  return(corriente);  
}


/*
 * sendData(): Envia datos a la nube ThingSpeak
 */
void sendData(String key, int field, double value){

  //Read IP address.
  gsm.SimpleWriteln("AT+CIFSR");
  delay(7000);
  //Read until serial buffer is empty.
  gsm.WhileSimpleRead();
     
  //String urlPath ="/update?api_key=" + key + "&field" + String(field) + "=" + String(value);
  String urlPath1 = key;
  String urlPath2 = "&field" + String(field) + "=" + String(value);
  String urlPath = "/update?api_key=" + urlPath1 + urlPath2;
  char str_array[urlPath.length()];
  urlPath.toCharArray(str_array, urlPath.length());
  Serial.println("GET Request:");
  Serial.println(str_array);

  //TCP Client GET, send a GET request to the server and
  //save the reply.
  //numdata=inet.httpGET("api.thingspeak.com", 80, "/update?api_key=IX1M46AF2ZWGIQYJ&field1=1.2345", msg, 200);
  numdata=inet.httpGET("api.thingspeak.com", 80, str_array, msg, 50);
  
  //Print the results.
  Serial.println("\nNumber of data received:");
  Serial.println(numdata);
  //Serial.println("\nData received:");
  //Serial.println(msg);
}

/*
 * serialhwread(): lee datos del modulo sim900 desde el serial
 */
void serialhwread()
{
     int i=0;
     if (Serial.available() > 0) {
          while (Serial.available() > 0) {
               inSerial[i]=(Serial.read());
               delay(10);
               i++;
          }

          inSerial[i]='\0';
          if(!strcmp(inSerial,"/END")) {
               Serial.println("_");
               inSerial[0]=0x1a;
               inSerial[1]='\0';
               gsm.SimpleWriteln(inSerial);
          }
          //Send a saved AT command using serial port.
          if(!strcmp(inSerial,"TEST")) {
               Serial.println("SIGNAL QUALITY");
               gsm.SimpleWriteln("AT+CSQ");
          }
          //Read last message saved.
          if(!strcmp(inSerial,"MSG")) {
               Serial.println(msg);
          } else {
               Serial.println(inSerial);
               gsm.SimpleWriteln(inSerial);
          }
          inSerial[0]='\0';
     }
}

