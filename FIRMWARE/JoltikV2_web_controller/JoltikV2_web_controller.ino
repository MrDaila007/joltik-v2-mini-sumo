// Joltik V2 Web Controller
// ESP8266 Wemos D1 Mini - WiFi configuration, telemetry & debug console
// Communicates with RP2040-Zero robot controller via serial

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WebSocketsServer.h>
#include <SoftwareSerial.h>
#include "web_ui.h"

// ---- WiFi AP settings ----
const char* AP_SSID = "Joltik-V2";
const char* AP_PASS = "joltik2024";

// ---- Robot serial pins ----
// D5 (GPIO14) = RX from robot, D6 (GPIO12) = TX to robot
#define ROBOT_RX_PIN D5
#define ROBOT_TX_PIN D6
#define ROBOT_BAUD 9600

// ---- Serial Protocol ----
// ESP -> RP2040 commands:
//   $SENSORS          - request sensor data
//   $DRIVE:left,right - set motor speeds (-255..255)
//   $STOP             - emergency stop
//   $START            - start robot (5s countdown)
//   $GETCFG           - request config
//   $SETCFG:dohyo,mode,lineEn - save config
//
// RP2040 -> ESP responses:
//   #S:fr,fl,r,l,lineR,lineL,state  - sensor data
//   #CFG:dohyo,mode,lineEn          - config data
//   #OK                              - command ack
//   (anything else)                  - debug message

SoftwareSerial robotSerial(ROBOT_RX_PIN, ROBOT_TX_PIN);
ESP8266WebServer server(80);
WebSocketsServer ws(81);

// ---- State ----
String rxBuffer;
unsigned long lastPoll = 0;
unsigned long lastRobotRx = 0;
bool robotConnected = false;

const unsigned long POLL_INTERVAL_MS = 150;
const unsigned long ROBOT_TIMEOUT_MS = 2000;

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Joltik V2 Web Controller ===");

  robotSerial.begin(ROBOT_BAUD);
  rxBuffer.reserve(256);

  // Start WiFi access point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("WiFi AP: %s  IP: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());

  // mDNS: access via http://joltik.local
  MDNS.begin("joltik");

  // HTTP server
  server.on("/", handleRoot);
  server.onNotFound(handleRoot);
  server.begin();

  // WebSocket server
  ws.begin();
  ws.onEvent(onWsEvent);

  Serial.println("Ready. Connect to WiFi and open http://joltik.local");
}

void loop() {
  ws.loop();
  server.handleClient();
  MDNS.update();

  readRobotSerial();

  // Periodic sensor poll
  if (millis() - lastPoll >= POLL_INTERVAL_MS) {
    lastPoll = millis();
    robotSerial.println("$SENSORS");
  }

  // Track robot connection status
  bool wasConnected = robotConnected;
  robotConnected = (millis() - lastRobotRx < ROBOT_TIMEOUT_MS);
  if (wasConnected != robotConnected) {
    String status = robotConnected ? "CONN:1" : "CONN:0";
    wsBroadcast(status);
  }
}

// ---- HTTP ----

void handleRoot() {
  server.send_P(200, "text/html", WEB_UI_HTML);
}

// ---- WebSocket ----

void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("[WS] Client %u connected\n", num);
      // Send current connection status
      ws.sendTXT(num, robotConnected ? "CONN:1" : "CONN:0");
      // Request config from robot
      robotSerial.println("$GETCFG");
      break;

    case WStype_DISCONNECTED:
      Serial.printf("[WS] Client %u disconnected\n", num);
      break;

    case WStype_TEXT:
      handleWebCommand(num, String((char*)payload));
      break;

    default:
      break;
  }
}

void handleWebCommand(uint8_t client, const String& msg) {
  Serial.printf("[WS] cmd: %s\n", msg.c_str());

  if (msg.startsWith("DRIVE:")) {
    robotSerial.println("$" + msg);
  }
  else if (msg == "STOP") {
    robotSerial.println("$STOP");
  }
  else if (msg == "START") {
    robotSerial.println("$START");
  }
  else if (msg.startsWith("SETCFG:")) {
    robotSerial.println("$" + msg);
  }
  else if (msg == "GETCFG") {
    robotSerial.println("$GETCFG");
  }
}

// ---- Robot serial ----

void readRobotSerial() {
  while (robotSerial.available()) {
    char c = robotSerial.read();
    if (c == '\n') {
      rxBuffer.trim();
      if (rxBuffer.length() > 0) {
        lastRobotRx = millis();
        processRobotMessage(rxBuffer);
      }
      rxBuffer = "";
    } else if (c != '\r') {
      if (rxBuffer.length() < 255) {
        rxBuffer += c;
      }
    }
  }
}

void processRobotMessage(const String& msg) {
  Serial.printf("[Robot] %s\n", msg.c_str());

  if (msg.startsWith("#S:")) {
    // Sensor data -> forward to web clients
    wsBroadcast("S:" + msg.substring(3));
  }
  else if (msg.startsWith("#CFG:")) {
    // Config data
    wsBroadcast("CFG:" + msg.substring(5));
  }
  else if (msg.startsWith("#OK")) {
    wsBroadcast("OK");
  }
  else {
    // Debug message
    wsBroadcast("D:" + msg);
  }
}

void wsBroadcast(const String& msg) {
  ws.broadcastTXT(msg.c_str(), msg.length());
}
