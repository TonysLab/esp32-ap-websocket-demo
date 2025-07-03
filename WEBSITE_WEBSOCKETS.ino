/*
  MIT License

  Copyright (c) 2025 Tony's Lab

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the “Software”), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#include <Arduino.h>             // Core Arduino functionality (GPIO, timing, Serial, etc.)
#include <WiFi.h>                // ESP32 Wi-Fi management (AP and Station modes)
#include "LittleFS.h"            // Flash-based file system for serving website files
#include <AsyncTCP.h>            // Non-blocking TCP library for asynchronous networking
#include <ESPAsyncWebServer.h>   // Asynchronous HTTP and WebSocket server built on AsyncTCP

#define trigPin 2
#define echoPin 1

// Configure SoftAP settings and network parameters:
// - SSID, password, channel, visibility, and max clients
// - Static IP, gateway, and subnet mask for hosting the web server
#define         AP_SSID               "TonysLab"
#define         AP_PASS               "TonysLab"
#define         AP_CHANNEL            1
#define         AP_HIDDEN             false
#define         AP_MAX_CON            8
IPAddress       local_IP              (192, 168, 4, 10);
IPAddress       gateway               (192, 168, 4, 9);
IPAddress       subnet                (255, 255, 255, 0);

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a WebSocket object
AsyncWebSocket ws("/ws");

unsigned long previousMillis = 0;
const long interval = 100;

// Initialize LittleFS
void initLittleFS() {
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
    File file = LittleFS.open("/index.html", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  file.close();
}

// Send WebSocket
void notifyClients(String MESSAGE) {
  ws.textAll(MESSAGE);
}

// Process incoming WebSocket text messages
// Parse "RGB:" commands, and update the RGB LED color accordingly

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    Serial.println(((char*)data));
    String msg = String((char*)data);

    int red = 0, green = 0, blue = 0;

    if (msg.startsWith("RGB:")) {
      msg.remove(0, 4);  // remove "RGB:"

      int firstComma = msg.indexOf(',');
      int secondComma = msg.indexOf(',', firstComma + 1);

      if (firstComma > 0 && secondComma > firstComma) {
        red   = msg.substring(0, firstComma).toInt();
        green = msg.substring(firstComma + 1, secondComma).toInt();
        blue  = msg.substring(secondComma + 1).toInt();
        
        rgbLedWrite(38, red, green, blue);
      }
    }    
  }
}

// Cases to happen on certain Wi-Fi Events
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

// Initialize the WebSocket endpoint used by the website’s JavaScript:
// This registers the “/ws” path so that client-side code can open a WebSocket
// connection to receive and send messages in real time via the onEvent handler.
void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}


// Sends a trigger pulse to the ultrasonic sensor, measures the echo return time,
// calculates the distance in centimeters based on the speed of sound,
// prints the measured distance, and returns it.
int Read_Distance(){
  long duration, distance;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration / 2) / 29.1;
  Serial.println("distance: ");
  Serial.println(distance);
  return distance;
}


void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(38, OUTPUT);

  initLittleFS();
  
  WiFi.softAPConfig(local_IP, gateway, subnet); // Apply the static network configuration for the ESP32 SoftAP
  WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, AP_HIDDEN, AP_MAX_CON); // Launch the SoftAP with specified credentials and settings
  Serial.print("IP address for network ");
  Serial.print(AP_SSID);
  Serial.print(" : ");
  Serial.print(WiFi.softAPIP());
  Serial.println("\n");
  initWebSocket();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) { // Serve the main page on HTTP GET requests to "/"
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.serveStatic("/", LittleFS, "/"); // Serve all other static assets (JS, CSS, images) from LittleFS
  server.begin();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    notifyClients(String(Read_Distance()));
  }

  ws.cleanupClients();
}
