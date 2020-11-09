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

#include <MeteoLabBeacon.h>

// BLE channel number (37-39)
const uint8_t channel[3]   = {37,38,39};

// physical frequency (2400+x MHz)
const uint8_t frequency[3] = { 2,26,80};


// This is a rather convoluted hack to extract the month number from the build date in
// the __DATE__ macro using a small hash function + lookup table. Since all inputs are
// const, this can be fully resolved by the compiler and saves over 200 bytes of code.
#define month(m) month_lookup[ (( ((( (m[0] % 24) * 13) + m[1]) % 24) * 13) + m[2]) % 24 ]
const uint8_t month_lookup[24] = { 0,6,0,4,0,1,0,17,0,8,0,0,3,0,0,0,18,2,16,5,9,0,1,7 };


// change buffer contents to "wire bit order"
void MeteoLabBeacon::swapbuf(uint8_t len) {
  uint8_t* buf = (uint8_t*)&buffer;
  while(len--) {
    uint8_t a = *buf;
    uint8_t v = 0;
    if (a & 0x80) v |= 0x01;
    if (a & 0x40) v |= 0x02;
    if (a & 0x20) v |= 0x04;
    if (a & 0x10) v |= 0x08;
    if (a & 0x08) v |= 0x10;
    if (a & 0x04) v |= 0x20;
    if (a & 0x02) v |= 0x40;
    if (a & 0x01) v |= 0x80;
    *(buf++) = v;
  }
}


// Constructor
MeteoLabBeacon::MeteoLabBeacon( RF24* _radio ):
	radio(_radio),
	current(0)
{ }


/* Simple converter from arduino float to a nRF_Float */
// Supports values from -167772 to +167772, with two decimal places
nRF_Float MeteoLabBeacon::to_nRF_Float(float t) {
  int32_t ret;
  int32_t exponent = -2;
  ret = ((exponent & 0xff) << 24) | (((int32_t)(t * 100)) & 0xffffff);
  return ret;
}


/* Set BLE radio parameters */
void MeteoLabBeacon::begin(const char* _name) {
  name = _name;
  radio->begin();
  // set standard parameters
  radio->setAutoAck(false);
  radio->setDataRate(RF24_1MBPS);
  radio->disableCRC();
  radio->setChannel( frequency[current] );
  radio->setRetries(0,0);
  radio->setPALevel(RF24_PA_MAX);
  // set advertisement address: 0x8E89BED6 (bit-reversed -> 0x6B7D9171)
  radio->setAddressWidth(4);
  radio->openReadingPipe(0,0x6B7D9171);
  radio->openWritingPipe(  0x6B7D9171);
  radio->powerUp();
}


/* Set the current channel (from 37 to 39) */
void MeteoLabBeacon::setChannel( uint8_t num ) {
  current = min(2,max(0,num-37));
  radio->setChannel( frequency[current] );
}


/* Hop to the next channel */
void MeteoLabBeacon::hopChannel() {
  current++;
  if(current >= sizeof(channel)) current = 0;
  radio->setChannel( frequency[current] );
}


// Broadcast an advertisement packet with optional payload
// Data type will be 0xFF (Manufacturer Specific Data)
bool MeteoLabBeacon::advertise( void* buf, uint8_t buflen ) {
  return advertise(0xFF, buf, buflen);
}


bool MeteoLabBeacon::addChunk(uint8_t chunk_type, uint8_t buflen, const void* buf) {
  if(buffer.pl_size + buflen + 2 > 21 + 6) // (buflen+2) is how much this chunk will take, 21 is payload size without crc and 6 is MAC size
    return false;
  btle_pdu_chunk* chunk = (btle_pdu_chunk*) (buffer.payload+buffer.pl_size-6);
  chunk->type = chunk_type;
  for(uint8_t i = 0; i < buflen; i++)
    chunk->data[i] = ((uint8_t*)buf)[i];
  chunk->size = buflen + 1;
  buffer.pl_size += buflen + 2;
  return true;
}


// Broadcast an advertisement packet with a specific data type
// Standardized data types can be seen here: 
// https://www.bluetooth.org/en-us/specification/assigned-numbers/generic-access-profile
bool MeteoLabBeacon::advertise(uint8_t data_type, void* buf, uint8_t buflen) {
  preparePacket();
  // add custom data, if applicable
  if(buflen > 0) {
    bool success = addChunk(data_type, buflen, buf);
    if(!success) {
      return false;
    }
  }
  transmitPacket();
  return true;
}


/* Prepare Packet */
void MeteoLabBeacon::preparePacket() {
  // insert pseudo-random MAC address
  buffer.mac[0] = ((__TIME__[6]-0x30) << 4) | (__TIME__[7]-0x30);
  buffer.mac[1] = ((__TIME__[3]-0x30) << 4) | (__TIME__[4]-0x30);
  buffer.mac[2] = ((__TIME__[0]-0x30) << 4) | (__TIME__[1]-0x30);
  buffer.mac[3] = ((__DATE__[4]-0x30) << 4) | (__DATE__[5]-0x30);
  buffer.mac[4] = month(__DATE__);
  buffer.mac[5] = ((__DATE__[9]-0x30) << 4) | (__DATE__[10]-0x30) | 0xC0; // static random address should have two topmost bits set

  buffer.pdu_type = 0x42;    // PDU type: ADV_NONCONN_IND, TX address is random
  buffer.pl_size = 6; //including MAC

  // add device descriptor chunk
  uint8_t flags = 0x05;
  addChunk(0x01, 1, &flags);

  // add "complete name" chunk
  if(strlen(name) > 0) {
    addChunk(0x09, strlen(name), name);
  }
}


/* Transmit Packet */
void MeteoLabBeacon::transmitPacket() {
  uint8_t pls = buffer.pl_size - 6;
  // calculate CRC over header+MAC+payload, append after payload
  uint8_t* outbuf = (uint8_t*)&buffer;
  crc( pls+8, outbuf+pls+8);
	
  // whiten header+MAC+payload+CRC, swap bit order
  whiten( pls+11 );
  swapbuf( pls+11 );
	
  // flush buffers and send
  radio->stopListening();
  radio->write( outbuf, pls+11 );
}


/* Listen for advertisement packets */
bool MeteoLabBeacon::listen(int timeout) {
  radio->startListening();
  delay(timeout);

  if(!radio->available())
    return false;

  uint8_t total_size = 0;
  uint8_t* inbuf = (uint8_t*)&buffer;

  while(radio->available()) {
    // fetch the payload, and check if there are more left
    radio->read( inbuf, sizeof(buffer) );
    // decode: swap bit order, un-whiten
    swapbuf( sizeof(buffer) );
    whiten( sizeof(buffer) );

    // size is w/o header+CRC -> add 2 bytes header
    total_size = inbuf[1]+2;
    uint8_t in_crc[3];

    // calculate & compare CRC
    crc( total_size, in_crc );
    for(uint8_t i = 0; i < 3; i++)
      if(inbuf[total_size+i] != in_crc[i])
        return false;
  }

  return true;
}


/* see BT Core Spec 4.0, Section 6.B.3.2 */
void MeteoLabBeacon::whiten( uint8_t len ) {
  uint8_t* buf = (uint8_t*)&buffer;

  // initialize LFSR with current channel, set bit 6
  uint8_t lfsr = channel[current] | 0x40;

  while(len--) {
    uint8_t res = 0;
    // LFSR in "wire bit order"
    for(uint8_t i = 1; i; i <<= 1) {
      if(lfsr & 0x01) {
        lfsr ^= 0x88;
        res |= i;
      }
      lfsr >>= 1;
    }
    *(buf++) ^= res;
  }
}


/* CRC Calculation
 * see BT Core Spec 4.0, Section 6.B.3.1.1
 */
void MeteoLabBeacon::crc(uint8_t len, uint8_t* dst) {
  uint8_t* buf = (uint8_t*)&buffer;

  // initialize 24-bit shift register in "wire bit order"
  // dst[0] = bits 23-16, dst[1] = bits 15-8, dst[2] = bits 7-0
  dst[0] = 0xAA;
  dst[1] = 0xAA;
  dst[2] = 0xAA;

  while(len--) {
    uint8_t d = *(buf++);
    for(uint8_t i = 1; i; i <<= 1, d >>= 1) {
      // save bit 23 (highest-value), left-shift the entire register by one
      uint8_t t = dst[0] & 0x01;        dst[0] >>= 1;
      if(dst[1] & 0x01) dst[0] |= 0x80; dst[1] >>= 1;
      if(dst[2] & 0x01) dst[1] |= 0x80; dst[2] >>= 1;
      // if the bit just shifted out (former bit 23) and the incoming data
      // bit are not equal (i.e. bit_out ^ bit_in == 1) => toggle tap bits
      if(t != (d & 1)) {
        // toggle register tap bits (=XOR with 1) according to CRC polynom
        dst[2] ^= 0xDA; // 0b11011010 inv. = 0b01011011 ^= x^6+x^4+x^3+x+1
        dst[1] ^= 0x60; // 0b01100000 inv. = 0b00000110 ^= x^10+x^9
      }
    }
  }
}


/* Read Atmega328P internal temperature sensor */
float MeteoLabBeacon::readTemperature() {
  float b = 125;
  float a = 9.30;
  // Read temperature sensor against 1.1V reference
  ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);
  delay(4);
  // Start AD conversion
  ADCSRA |= _BV(ADEN);
  delay(20);
  // Start the ADC
  ADCSRA |= _BV(ADSC);
  // Detect end-of-conversion
  while(bit_is_set(ADCSRA,ADSC));
  // Raw data
  long raw = ADCL | (ADCH << 8);
  return (raw - b) / a;
}


/* Power Supply Voltage */
long MeteoLabBeacon::getVoltage() {
  // Internal Voltage (mV)
  long InternalReferenceVoltage = 1062;
  // REFS0 : Selects AVcc external reference
  // MUX3 MUX2 MUX1 : Selects 1.1V (VBG) 
  ADMUX = bit (REFS0) | bit (MUX3) | bit (MUX2) | bit (MUX1);
  ADCSRA |= bit( ADSC );  // start conversion
  while(ADCSRA & bit (ADSC)) { }  // wait for conversion to complete (toss this measurement)
  ADCSRA |= bit( ADSC );  // start conversion
  while(ADCSRA & bit (ADSC)) { }  // wait for conversion to complete
  // Power Supply Voltage (mV)
  long voltage = (((InternalReferenceVoltage * 1024) / ADC) + 5);
  return voltage;
}


/* Battery Capacity */
long MeteoLabBeacon::getBatteryCapacity(long voltage) {
  long minVoltage = 3600;
  long maxVoltage = 4200;
  long capacity;
  if(voltage < minVoltage) {
    capacity = 0;
  }
  else if(voltage > maxVoltage) {
    capacity = 100;
  }
  else {
    capacity = map(voltage, minVoltage, maxVoltage, 0, 100);
  }
  return capacity;
}



void MeteoLabBeacon::sleep(uint8_t time) {
  // Switch BLE power down
  radio->powerDown();
  // Set most power saving mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  // Turn off ADC
  ADCSRA &= ~(1 << ADEN);
  // Set watchdog timer
  wdt_enable(time);
  // Allow watchdog interraption
  WDTCSR |= (1 << WDIE);  
  cli();
  sleep_enable();
  sleep_bod_disable();
  sei();
  // Go to sleep mode
  sleep_cpu();
  // ... sleeping here
  sleep_disable();
  // Turn on ADC
  ADCSRA |= (1 << ADEN);
  // Switch BLE power up
  radio->powerUp();
}


/* Broadcast Data */
bool MeteoLabBeacon::broadcast(int16_t uuid, float value) {
  nrf_service_data data;
  data.service_uuid = uuid;
  data.value = to_nRF_Float(value);
  advertise(0x16, &data, sizeof(data));
  hopChannel();
  advertise(0x16, &data, sizeof(data));
  hopChannel();
  advertise(0x16, &data, sizeof(data));
  hopChannel();
  return true;
}


/* Broadcast Battery Level Data */
bool MeteoLabBeacon::broadcastBatteryCapacity() {
  long voltage = getVoltage();
  long level = getBatteryCapacity(voltage);
  battery_level_data battery_data;
  battery_data.service_uuid = NRF_BATTERY_SERVICE_UUID;
  battery_data.battery_percentage = level;
  advertise(0x16, &battery_data, sizeof(battery_data));
  hopChannel();
  advertise(0x16, &battery_data, sizeof(battery_data));
  hopChannel();
  advertise(0x16, &battery_data, sizeof(battery_data));
  hopChannel();
  return true;
}

