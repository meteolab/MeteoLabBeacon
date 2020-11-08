#include <RF24.h>
#include <SPI.h>
#include <MeteoLabBeacon.h>
#include "DHT.h"


/*

  ----------------------    ----------------
  |GND CE  SCK  MISO|  |    |VCC DATA - GND|
  |VCC CSN MOSI IRQ |  |    |--------------|
  |-----------------   |    |              |
  |                    |    |              |
  |                    |    |              |
  |                    |    |              |
  |                    |    |              |
  |                    |    |              |
  |      (((*)))       |    |              |
  ----------------------    ----------------
      nRF24L01+ pins           DHT22 pins


  -------------------------------------------------------------------------------------
 |              |                    nRF24L01+                     |        DHT22      |
 |    Board     |--------------------------------------------------|-------------------|
 |              |  GND   CE    SCK   MISO  VCC   CSN   MOSI  IRQ   |  VCC   DATA  GND  |
 |-------------------------------------------------------------------------------------|
 | Arduino Nano |  GND   9     13    12    3V3   10    11    2     |  5V    3     GND  |
 | Arduino Uno  |  GND   9     13    12    3V3   10    11    2     |  5V    3     GND  |
 | Arduino Mega |  GND   9     13    12    3V3   10    11    2     |  5V    3     GND  |
  -------------------------------------------------------------------------------------

*/

#define NRF_CE  9
#define NRF_CSN 10
#define DHTPIN  3
#define DHTTYPE DHT22

RF24 radio(NRF_CE, NRF_CSN);
MeteoLabBeacon beacon(&radio);
DHT dht(DHTPIN, DHTTYPE);


ISR(WDT_vect) {
  // Disable watchdog timer after interraption
  wdt_disable();
}


void setup() {

  // Start BLE beacon
  beacon.begin("MeteoLab");

  // Start DHT sensor
  dht.begin();

}



void loop() {

  // Read Sensor Data
  float t = dht.readTemperature();
  float rh = dht.readHumidity();

  // Broadcast Data
  beacon.broadcast(NRF_TEMPERATURE_UUID, t);
  beacon.broadcast(NRF_HUMIDITY_UUID, rh);
  beacon.broadcastBatteryCapacity();

  // Sleep for one of possible periods: 15 ms, 30 ms, 60 ms, 120 ms, 250 ms, 500 ms, 1 s, 2 s
  beacon.sleep(WDTO_2S);

}
