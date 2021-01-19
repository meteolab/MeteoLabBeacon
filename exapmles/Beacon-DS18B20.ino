#include <RF24.h>
#include <SPI.h>
#include <MeteoLabBeacon.h>
#include <OneWire.h>
#include <DallasTemperature.h>


/*

  ----------------------    --------------
  |GND CE  SCK  MISO|  |    |GND  DQ  VDD|
  |VCC CSN MOSI IRQ |  |    |------------|
  |-----------------   |    |            |
  |                    |    |            |
  |                    |    |            |
  |                    |    |            |
  |                    |    |            |
  |                    |    |            |
  |      (((*)))       |    |            |
  ----------------------    --------------
      nRF24L01+ pins          DS18B20 pins

  -------------------------------------------------------------------------------------
 |              |                    nRF24L01+                     |      DS18B20      |
 |    Board     |--------------------------------------------------|-------------------|
 |              |  GND   CE    SCK   MISO  VCC   CSN   MOSI  IRQ   |  GND   DQ    VDD  |
 |-------------------------------------------------------------------------------------|
 | Arduino Nano |  GND   9     13    12    3V3   10    11    2     |  GND   4     5V   |
 | Arduino Uno  |  GND   9     13    12    3V3   10    11    2     |  GND   4     5V   |
 | Arduino Mega |  GND   9     13    12    3V3   10    11    2     |  GND   4     5V   |
  -------------------------------------------------------------------------------------

*/

#define NRF_CE  9
#define NRF_CSN 10
#define ONE_WIRE_BUS 4

RF24 radio(NRF_CE, NRF_CSN);
MeteoLabBeacon beacon(&radio);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


ISR(WDT_vect) {
  // Disable watchdog timer after interraption
  wdt_disable();
}


void setup() {

  // Start BLE beacon
  beacon.begin("MeteoLab");

  // Start DS18B20 sensor
  sensors.begin();

}



void loop() {

  // Read Sensor Data
  sensors.requestTemperatures();
  float t = sensors.getTempCByIndex(0);

  // Broadcast Data
  beacon.broadcast(NRF_TEMPERATURE_UUID, t);
  beacon.broadcastBatteryCapacity();

  // Sleep for one of possible periods: 15 ms, 30 ms, 60 ms, 120 ms, 250 ms, 500 ms, 1 s, 2 s
  beacon.sleep(WDTO_2S);

}
