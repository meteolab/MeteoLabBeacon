MeteoLabBeacon
====

Arduino library for Bluetooth Low Energy (BLE) environmental sensor communication using the nRF24L01+

The library is based on code by

 * 2012 Dmitry Grinberg, http://dmitry.gr/index.php?r=05.Projects&proj=11.%20Bluetooth%20LE%20fakery
 * 2013 Florian Echtler, https://github.com/floe/BTLE
 * 2019 Pawel Hernik, https://github.com/cbm80amiga/BLE_beacon

and licensed under GPLv3 except functions written by Dmitry Grinberg under a separate
license (see link above for details).

The simplest BLE temperature beacon consists of an Arduino Nano/Uno/Mega board and a nRF24L01+ radio module. It is also possible to connect sensors to measure:
* temperature DS18B20
* temperature/humidity DHT22
* temperature/humidity/pressure BME280

The measurement data is displayed on your smartphone using a web application:
https://app.meteolab.org

<img src="https://meteolab.org/assets/images/meteolab-app-03.png">
