#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"

#define trigPin 2
#define echoPin 1

// Setting The AP Mode Credential
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

void notifyClients(String MESSAGE) {
  ws.textAll(MESSAGE);
}

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

// Cases to happen on certain Events
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

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

int Read_Distance(){
  long duration, distance;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration/2) / 29.1;
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
  
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, AP_HIDDEN, AP_MAX_CON);
  Serial.print("IP address for network ");
  Serial.print(AP_SSID);
  Serial.print(" : ");
  Serial.print(WiFi.softAPIP());
  Serial.println("\n");
  initWebSocket();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.serveStatic("/", LittleFS, "/");
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