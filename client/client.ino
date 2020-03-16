#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <SSD1306Wire.h>
#include <Arduino_JSON.h>

// Print messages to Serial
#define DEBUG_MODE true

// GPIO the display is attached to
#define pinDisplaySDA D2
#define pinDisplaySCL D1

// Credentials for Wi-Fi
const char* ssid     = "AGS-WiFi";
const char* password = "qwepoi123654789";

// Endpoints
const char* endpointStatus = "http://192.168.1.13/status";

const long interval = 2 * 1000;
unsigned long previousMillis = 0;
bool doReconnect = false;

int servoDegreeCurrent;
int servoDegreeMin;
int servoDegreeMax;
int progress;

SSD1306Wire display(0x3c, pinDisplaySDA, pinDisplaySCL);

void setup() {
  delay(2000);

  if (DEBUG_MODE) {
    Serial.begin(74880);
  }
  println("Debug mode is enabled");
  println("Starting...");

  initDisplay();
  initWiFi();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    if (WiFi.isConnected()) {
      processing();

      // save the last HTTP GET Request
      previousMillis = currentMillis;
    }
    else {
      doReconnect = true;
      println("WiFi Disconnected");
    }

    if (doReconnect) {
        WiFi.reconnect();
        connectionWiFi();
        doReconnect = false;
        println("WiFi Reconnected");
    }
  }
}

// Processing
void processing() {
  JSONVar payload = httpGETRequest(endpointStatus);

  if (payload.hasOwnProperty("servoDegreeCurrent")
      && payload.hasOwnProperty("servoDegreeMin")
      && payload.hasOwnProperty("servoDegreeMax"
  )) {
    servoDegreeCurrent = payload["servoDegreeCurrent"];
    servoDegreeMin = payload["servoDegreeMin"];
    servoDegreeMax = payload["servoDegreeMax"];
    progress = (servoDegreeCurrent - servoDegreeMin) * 100 / (servoDegreeMax - servoDegreeMin);

    println("Servo rotated: " + String(servoDegreeCurrent) + " | Progress: " + String(progress));
    println("Rotation min: " + String(servoDegreeMin) + " | Rotation max: " + String(servoDegreeMax));

    getDisplayHeader();
    display.setFont(ArialMT_Plain_16);
    display.drawString(54, 30, String(servoDegreeCurrent) + "°");
    display.drawProgressBar(0, 46, 127, 10, progress);
    display.display();
  }
}

// Send GET request
JSONVar httpGETRequest(const char* endpoint) {
  WiFiClient client;
  HTTPClient http;

  String data = "";
  if (http.begin(client, endpoint)) {
    int httpResponseCode = http.GET();
    println("Response code: " + String(httpResponseCode));

    if (httpResponseCode > 0) {
      data = http.getString();
    } else {
      doReconnect = true;
      println("Bad request");
    }
  }
  else {
    doReconnect = true;
    println("Unable to connect");
  }

  // Free resources
  http.end();

  JSONVar payload = JSON.parse(data);

  return payload;
}

// Get header for display
void getDisplayHeader() {
  display.setFont(ArialMT_Plain_10);
  display.clear();
  display.drawString(0, 0, "AP SSID: " + String(ssid));
  display.drawString(0, 13, "Client IP: " + WiFi.localIP().toString());
}

// Get connection status as string
void getConnectionStatus(int status) {
  String statusString = "Unknown";

  switch (status)
  {
    case WL_NO_SHIELD:        // 255
      statusString = "WL_NO_SHIELD";
      break;
    case WL_IDLE_STATUS:      // 0
      statusString = "WL_IDLE_STATUS";
      break;
    case WL_NO_SSID_AVAIL:    // 1
      statusString = "WL_NO_SSID_AVAIL";
      break;
    case WL_SCAN_COMPLETED:   // 2
      statusString = "WL_SCAN_COMPLETED";
      break;
    case WL_CONNECTED:        // 3
      statusString = "WL_CONNECTED";
      break;
    case WL_CONNECT_FAILED:   // 4
      statusString = "WL_CONNECT_FAILED";
      break;
    case WL_CONNECTION_LOST:  // 5
      statusString = "WL_CONNECTION_LOST";
      break;
    case WL_DISCONNECTED:     // 6
      statusString = "WL_DISCONNECTED";
      break;
  }

  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Connecting to " + String(ssid));
  display.drawString(0, 13, statusString);
  display.display();

  println("Connecting to " + String(ssid) + "... " + statusString);
}

// Connect to Wi-Fi
void connectionWiFi() {
  getConnectionStatus(WiFi.status());
  while (!WiFi.isConnected()) {
    delay(750);
    getConnectionStatus(WiFi.status());
  }
}

// Wi-Fi initialization
void initWiFi() {
  // Connect to Wi-Fi network with SSID and password
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);

  connectionWiFi();

  getDisplayHeader();
  display.display();

  // Print local IP address and start web server
  println("");
  println("WiFi connected.");
  println("Client IP: " + WiFi.localIP().toString());
}

// Display initialization
void initDisplay() {
  display.init();
  display.clear();
  display.flipScreenVertically();

  println("Display initialized");
}

// Print message to Serial
void println(String message) {
  if (DEBUG_MODE) {
    Serial.println(message);
  }
}
