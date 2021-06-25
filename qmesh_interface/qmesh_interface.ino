

#include "BluetoothSerial.h"
#include <deque>
#include <vector>
#include <esp_wifi.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
  

#define SERIAL_TCP_PORT 8880
const char* ssid = "";
const char* password =  "";
const char* mdns_name = "QMesh-Node0";

WiFiServer wifiServer(SERIAL_TCP_PORT);

#define RXD2 16
#define TXD2 17


void setup() {
  Serial2.begin(230400, SERIAL_8N1, RXD2, TXD2);
  Serial2.setRxBufferSize(8192);
  Serial.begin(230400);

  delay(1000);

  WiFi.setHostname(mdns_name);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");
  Serial.println(WiFi.localIP());

  wifiServer.begin();

  if (!MDNS.begin(mdns_name)) {
    Serial.println("Error starting mDNS");
    return;
  }

  delay(500);
  Serial.println("Done with everything");
}


void loop() {

  WiFiClient client = wifiServer.available();
  if (client) {
    while (client.connected()) {
      while (client.available() > 0) {
        //Serial.println("Receiving data");
        char c = client.read();
        Serial2.write(c);
      }
      delay(10);
      if (Serial2.available()) {
        while (Serial2.available()) {
          //Serial.println("Sending data");
          client.write(Serial2.read());
        }
      }
    }
    client.stop();
    Serial.println("Client disconnected");
  }
}
