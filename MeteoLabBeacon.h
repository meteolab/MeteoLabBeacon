/*
 * Copyright (C) 2020 MeteoLab LLC <hello@meteolab.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 *
 * The library is based on code by
 *  2012 Dmitry Grinberg, http://dmitry.gr/index.php?r=05.Projects&proj=11.%20Bluetooth%20LE%20fakery
 *  2013 Florian Echtler, https://github.com/floe/BTLE
 *  2019 Pawel Hernik, https://github.com/cbm80amiga/BLE_beacon
 */


#ifndef _METEOLABBEACON_H_
#define _METEOLABBEACON_H_

#include "Arduino.h"
#include <RF24.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>

// IEEE-11073 32bit float format
typedef int32_t nRF_Float;

// Service UUIDs
#define NRF_TEMPERATURE_UUID			0x2A6E
#define NRF_HUMIDITY_UUID			0x2A6F
#define NRF_PRESSURE_UUID			0x2A6D
#define NRF_BAROMETRIC_PRESSURE_TREND_UUID	0x2AA3
#define NRF_RAINFALL_UUID			0x2A78
#define NRF_POLLEN_CONCENTRATION_UUID		0x2A75

// Elevation + Height = Altitude
#define NRF_ELEVATION_UUID			0x2A6C
#define NRF_HEIGHT_UUID				0x2A8E
#define NRF_ALTITUDE_UUID			0x2AB3
#define NRF_LATITUDE_UUID			0x2AAE
#define NRF_LONGITUDE_UUID			0x2AAF
#define NRF_TX_POWER_LEVEL_UUID			0x2A07

#define NRF_TEMPERATURE_SERVICE_UUID		0x1809
#define NRF_BATTERY_SERVICE_UUID		0x180F
#define NRF_DEVICE_INFORMATION_SERVICE_UUID	0x180A
#define NRF_EDDYSTONE_SERVICE_UUID		0xFEAA

#define EDDYSTONE_UID_FRAME_TYPE		0x00
#define EDDYSTONE_URL_FRAME_TYPE		0x10
#define EDDYSTONE_TLM_FRAME_TYPE		0x20
#define EDDYSTONE_EID_FRAME_TYPE		0x30

#define EDDYSTONE_HTTPWWW_URL_SCHEME		0x00  //stands for "http://www."
#define EDDYSTONE_HTTPSWWW_URL_SCHEME		0x01  //stands for "https://www."
#define EDDYSTONE_HTTP_URL_SCHEME		0x02  //stands for "http://"
#define EDDYSTONE_HTTPS_URL_SCHEME		0x03  //stands for "https://"

// Struct for sending temperature, humidity, pressure and etc.
struct nrf_service_data {
  int16_t   service_uuid;
  nRF_Float value;
};

// Struct for sending battery capacity
struct battery_level_data {
  uint16_t service_uuid;
  uint8_t battery_percentage;
};

#define EDDYSTONE_URL_HEADER_LENGTH (5)
struct eddystone_url_service_data {
  uint16_t service_uuid;
  uint8_t frame_type;
  int8_t tx_power; // take -20 if unsure
  uint8_t url_scheme;
  uint8_t encoded_url[11]; //11 bytes at max with current implementation. see https://github.com/google/eddystone/tree/master/eddystone-url#eddystone-url-http-url-encoding
};

// advertisement PDU
struct btle_adv_pdu {
  // packet header
  uint8_t pdu_type; // PDU type
  uint8_t pl_size;  // payload size
  // MAC address
  uint8_t mac[6];
  // payload (including 3 bytes for CRC)
  uint8_t payload[24];
};

// payload chunk in advertisement PDU payload
struct btle_pdu_chunk {
  uint8_t size;
  uint8_t type;
  uint8_t data[];
};


class MeteoLabBeacon {
  public:
    MeteoLabBeacon( RF24* _radio );
    // convert an arduino float to a nRF_Float
    static nRF_Float to_nRF_Float(float t);
    float readTemperature();
    static long getVoltage();
    static long getBatteryCapacity(long voltage);
    void begin( const char* _name ); // set BTLE-compatible radio parameters & name
    void setChannel( uint8_t num ); // set the current channel (from 36 to 38)
    void hopChannel();              // hop to the next channel
    // Broadcast an advertisement packet with a specific data type
    // Standardized data types can be seen here: 
    // https://www.bluetooth.org/en-us/specification/assigned-numbers/generic-access-profile
    bool advertise( uint8_t data_type, void* buf, uint8_t len ); 
    // Broadcast an advertisement packet with optional payload
    // Data type will be 0xFF (Manufacturer Specific Data)
    bool advertise( void* buf, uint8_t len ); 
    bool listen( int timeout = 100 );         // listen for advertisement packets (if true: result = buffer)
    btle_adv_pdu buffer;  // buffer for received BTLE packet (also used for outgoing!)
    void preparePacket();
    bool addChunk(uint8_t chunk_type, uint8_t buflen, const void* buf);
    void transmitPacket();
    void sleep(uint8_t time);
    bool broadcast(int16_t uuid, float value);
    bool broadcastBatteryCapacity();
  private:
    void whiten( uint8_t len );
    void swapbuf( uint8_t len );
    void crc( uint8_t len, uint8_t* dst );
    RF24* radio;       // pointer to the RF24 object managing the radio
    uint8_t current;   // current channel index
    const char* name;  // name of local device
};

#endif

