#include <RF24.h>
#include <SPI.h>
#include <MeteoLabBeacon.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

/*
  ----------------------    -----------------
  |GND CE  SCK  MISO|  |    |VIN GND SCL SDA|
  |VCC CSN MOSI IRQ |  |    |               |
  |-----------------   |    |               |
  |                    |    |               |
  |                    |    |               |
  |                    |    |               |
  |                    |    |               |
  |                    |    |               |
  |      (((*)))       |    |               |
  ----------------------    -----------------
      nRF24L01+ pins           BME280 pins


 --------------------------------------------------
|  nRF24L01+ |  Arduino   Arduino  Arduino         |
|            |  Uno/Nano  Mega     Leonardo/Micro  |
 --------------------------------------------------
|  GND       |  GND       GND      GND             |
|  CE        |  9         9        9               |
|  SCK       |  13        13       13              |
|  MISO      |  12        12       12              |
|  IRQ       |  2         2        2               |
|  MOSI      |  11        11       11              |
|  CSN       |  10        10       10              |
|  VCC       |  3V3       3V3      3V3             |
 --------------------------------------------------
|  BME280    |  Arduino   Arduino  Arduino         |
|            |  Uno/Nano  Mega     Leonardo/Micro  |
 --------------------------------------------------
|  VIN       |  3V3       3V3      3V3             |
|  GND       |  GND       GND      GND             |
|  SCL       |  A5        21       3               |
|  SDA       |  A4        20       2               |
 --------------------------------------------------
*/

#define NRF_CE  9
#define NRF_CSN 10

RF24 radio(NRF_CE, NRF_CSN);
MeteoLabBeacon beacon(&radio);
Adafruit_BME280 bme;


// Disable watchdog timer after interraption
ISR(WDT_vect) {
  wdt_disable();
}


void setup() {

  // Start BLE beacon
  beacon.begin("MeteoLab");

  // Start BME280 sensor
  if(!bme.begin(0x76) && !bme.begin(0x77)) {
    while(1);
  }

}



void loop() {

  // Read Sensor Data
  float t  = bme.readTemperature();
  float rh = bme.readHumidity();
  float p  = bme.readPressure();

  // Broadcast Data
  beacon.broadcast(NRF_TEMPERATURE_UUID, t);
  beacon.broadcast(NRF_HUMIDITY_UUID, rh);
  beacon.broadcast(NRF_PRESSURE_UUID, p);
  beacon.broadcastBatteryCapacity();

  // Sleep for one of possible periods: 15 ms, 30 ms, 60 ms, 120 ms, 250 ms, 500 ms, 1 s, 2 s
  beacon.sleep(WDTO_1S);

}
