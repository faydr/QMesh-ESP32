

#include "BluetoothSerial.h"
#include <esp_wifi.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <esp_task_wdt.h>


#define WDT_TIMEOUT 10
#define XFER_CHUNK_SIZE 256

#define SERIAL_TCP_PORT 8880
const char* ssid = "";
const char* password =  "";
const char* mdns_name = "QMesh-Node1";

WiFiServer wifiServer(SERIAL_TCP_PORT);

#define RXD2 16
#define TXD2 17


void setup() {
  Serial2.begin(230400, SERIAL_8N1, RXD2, TXD2);
  Serial2.setRxBufferSize(8192);
  Serial.begin(230400);

  delay(1000);

  Serial.println("Configuring WDT...");
  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch

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
  ArduinoOTA.begin();
  Serial.println("Done with everything");
}


int last = millis();
void loop() {
  ArduinoOTA.handle();

  // resetting WDT every 1s
  if (millis() - last >= 1000) {
    Serial.println("Resetting WDT...");
    esp_task_wdt_reset();
    last = millis();
  }
  WiFiClient client = wifiServer.available();
  if (client) {
    while (client.connected()) {
      int bytes_sent = 0;
      while (client.available() > 0) {
        //Serial.println("Receiving data");
        char c = client.read();
        Serial2.write(c);
        if (bytes_sent++ > XFER_CHUNK_SIZE) {
          break;
        }
      }
      delay(25);
      int bytes_recvd = 0;
      while (Serial2.available()) {
        //Serial.println("Sending data");
        client.write(Serial2.read());
        if (bytes_recvd++ > XFER_CHUNK_SIZE) {
          break;
        }
      }
      if (millis() - last >= 1000) {
        Serial.println("Resetting WDT...");
        esp_task_wdt_reset();
        last = millis();
      }
      delay(25);
    }
    client.stop();
    Serial.println("Client disconnected");
    delay(25);
    if (!client.connected()) {
      int bytes_recvd = 0;
      while (Serial2.available()) {
        //Serial.println("Sending data");
        Serial2.read();
        if (bytes_recvd++ > XFER_CHUNK_SIZE) {
          break;
        }
      }
    }
    // Reboot after a disconnect, to get rid of any cruft
    while(1);
  }
  else { // Empty the serial port buffer if there's no client connected
    int bytes_recvd = 0;
    while (Serial2.available()) {
      //Serial.println("Sending data");
      Serial2.read();
      if (bytes_recvd++ > XFER_CHUNK_SIZE) {
        break;
      }
    }
  }
  delay(25);
}
