//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial


#include "BluetoothSerial.h"
#include <deque>
#include <vector>
#include <esp_wifi.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>


#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

// For AP mode:
#define SERIAL_TCP_PORT 8880
const char *ssid = "QMesh0-WiFi";  // You will connect your phone to this Access Point
const char *pw = "password"; // and this is the password
IPAddress ip(192, 168, 4, 1);
IPAddress netmask(255, 255, 255, 0);
WiFiServer server(SERIAL_TCP_PORT);

volatile bool bt_active;

#define RXD2 16
#define TXD2 17


void setup() {
  Serial2.begin(230400, SERIAL_8N1, RXD2, TXD2);
  Serial2.setRxBufferSize(4096);  
  Serial.begin(230400);
  SerialBT.begin("QMesh0-BT"); // Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
  bt_active = false;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pw); // configure ssid and password for softAP
  delay(2000); // VERY IMPORTANT
  WiFi.softAPConfig(ip, ip, netmask); // configure ip address for softAP

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      }
      else { // U_SPIFFS
        type = "filesystem";
      }

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request

  ArduinoOTA.begin();

  if(!MDNS.begin("QMesh-Node0")) {
       Serial.println("Error starting mDNS");
       return;
  }

  delay(500);
}


uint8_t bt_buf[256];
#define MAX_DEQUE_SIZE 65536
#define MAX_WRITE_BYTES 16
std::deque<uint8_t> rx_serial, rx_btserial;

void loop() {
  // Handle OTA
  ArduinoOTA.handle();
  
  // Clear out any buffers if the Bluetooth isn't active
  if(!SerialBT.hasClient() && rx_btserial.size() != 0) {
    rx_btserial.clear();
    delay(20);
  }
  
  // First, store any received data
  if(Serial2.available()) {
    while(Serial2.available()) {
      rx_serial.push_front(Serial2.read());
      if(rx_serial.size() > MAX_DEQUE_SIZE) {
        rx_serial.clear();
      }
    }
  }
  
  if(SerialBT.hasClient()) {
    if(SerialBT.available()) {
      while(SerialBT.available()) {
        rx_btserial.push_front(SerialBT.read());
        if(rx_btserial.size() > MAX_DEQUE_SIZE) { // Flush buffer if it overfills
          rx_btserial.clear();
        }
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
    Serial2.write(write_bytes.data(), write_bytes.size());
  }

  if(SerialBT.hasClient()) {
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
  
}
