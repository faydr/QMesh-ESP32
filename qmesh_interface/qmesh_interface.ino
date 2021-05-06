//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial

#include "BluetoothSerial.h"
#include <deque>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;
//HardwareSerial Serial;


void setup() {
  Serial.begin(115200);
  Serial.setRxBufferSize(4096);
  SerialBT.begin("ESP32test"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
}

uint8_t bt_buf[256];
#define MAX_DEQUE_SIZE 65536
#define MAX_WRITE_BYTES 16
std::deque<uint8_t> rx_serial, rx_btserial;

void loop() {
  // First, store any received data
  if(Serial.available()) {
    while(Serial.available()) {
      rx_serial.push_front(Serial.read());
      if(rx_serial.size() > MAX_DEQUE_SIZE) {
        rx_serial.clear();
      }
    }
  }
  if(SerialBT.available()) {
    while(SerialBT.available()) {
      rx_btserial.push_front(SerialBT.read());
      if(rx_btserial.size() > MAX_DEQUE_SIZE) {
        rx_btserial.clear();
      }
    }
  }

  // Next, send out the buffer data
  if(rx_btserial.size() > 0) {
    std::vector<uint8_t> write_bytes;
    size_t num_write_bytes = Serial.availableForWrite();
    if(num_write_bytes > MAX_DEQUE_SIZE) {
      num_write_bytes = MAX_DEQUE_SIZE; 
    }
    if(num_write_bytes > rx_btserial.size()) {
      num_write_bytes = rx_btserial.size();
    }
    for(size_t i = 0; i < num_write_bytes; i++) {
      write_bytes.push_back(rx_btserial.back());
      rx_btserial.pop_back();
    }
    Serial.write(write_bytes.data(), write_bytes.size());
  }

  if(rx_serial.size() > 0) {
    std::vector<uint8_t> write_bytes;
    size_t num_write_bytes = rx_serial.size();
    if(num_write_bytes > MAX_DEQUE_SIZE) {
      num_write_bytes = MAX_DEQUE_SIZE; 
    }
    std::deque<uint8_t>::iterator iter = rx_serial.end();
    for(size_t i = 0; i < num_write_bytes; i++) {
      write_bytes.push_back(*--iter);
    }
    if(SerialBT.write(write_bytes.data(), write_bytes.size())) {
      rx_serial.erase(iter, rx_serial.end());
    }
  }
  
}
