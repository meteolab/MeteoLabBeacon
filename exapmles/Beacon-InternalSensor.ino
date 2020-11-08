#include <RF24.h>
#include <SPI.h>
#include <MeteoLabBeacon.h>

/*
  ----------------------
  |GND CE  SCK  MISO|  |
  |VCC CSN MOSI IRQ |  |
  |-----------------   |
  |                    |
  |                    |
  |                    |
  |                    |
  |                    |
  |      (((*)))       |
  ----------------------
      nRF24L01+ pins


  -----------------------------------------------------------------
 |              |                    nRF24L01+                     |
 |    Board     |--------------------------------------------------|
 |              |  GND   CE    SCK   MISO  VCC   CSN   MOSI  IRQ   |
 |-----------------------------------------------------------------|
 | Arduino Nano |  GND   9     13    12    3V3   10    11    2     |
 | Arduino Uno  |  GND   9     13    12    3V3   10    11    2     |
 | Arduino Mega |  GND   9     13    12    3V3   10    11    2     |
  -----------------------------------------------------------------

*/

#define NRF_CE  9
#define NRF_CSN 10

RF24 radio(NRF_CE, NRF_CSN);
MeteoLabBeacon beacon(&radio);


ISR(WDT_vect) {
  // Disable watchdog timer after interraption
  wdt_disable();
}


void setup() {

  // Start BLE beacon
  beacon.begin("MeteoLab");

}



void loop() {

  // Read Internal Sensor Data
  float t = beacon.readTemperature();

  // Broadcast Data
  beacon.broadcast(NRF_TEMPERATURE_UUID, t);
  beacon.broadcastBatteryCapacity();

  // Sleep for one of possible periods: 15 ms, 30 ms, 60 ms, 120 ms, 250 ms, 500 ms, 1 s, 2 s
  beacon.sleep(WDTO_2S);

}
