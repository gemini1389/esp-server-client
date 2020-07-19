#include <Servo.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Arduino_JSON.h>

// Print messages to Serial
#define DEBUG_MODE true

// GPIO the servo is attached to
#define pinServo D4
#define pinServoAnalog A0

// Config for servo
#define servoDelay 8
#define servoMinUs 720  // 0
#define servoMaxUs 2400 // 180

// Min and max degree for rotation servo
#define servoDegreeMin  45  // 0
#define servoDegreeMax  105 // 170
#define servoAnalogMin  348 // 221
#define servoAnalogMax  493 // 665
#define servoAnalogDiff 5   // degrees

#define servoRotationMaxAttempts 3

// Credentials for Wi-Fi
const char* ssid     = "AGS-WiFi";
const char* password = "qwepoi123654789";

// Config for network
IPAddress local_IP(192, 168, 1, 13);
IPAddress gateway(192, 168, 1, 13);
IPAddress subnet(255, 255, 255, 0);

int newDegree = servoDegreeMin;
int analogDegree;

Servo servo;
ESP8266WebServer server(80);

void setup() {
  delay(2000);

  if (DEBUG_MODE) {
    Serial.begin(115200);
  }
  println("Debug mode is enabled");
  println("Starting...");

  initWiFi();
  initServo();
  initServer();
}

void loop() {
  server.handleClient();
}

// Main page
void handleRoot() {
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head>\n";
  ptr += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
  ptr += "<link rel=\"icon\" href=\"data:,\">\n";

  ptr += "<style>body { text-align: center; font-family: \"Trebuchet MS\", Arial; margin-left:auto; margin-right:auto;}\n";
  ptr += ".slider { width: 300px; }</style>\n";

  // Web Page
  ptr += "</head><body><h1>ESP8266 with Servo</h1>\n";
  ptr += "<p>Position: <span id=\"servoPos\"></span></p>\n";
  ptr += "<input type=\"range\" min=\"" + String(servoDegreeMin) + "\" max=\"" + String(servoDegreeMax) + "\" class=\"slider\" id=\"servoSlider\" value=\"" + String(newDegree) + "\"/>";

  ptr += "<script>\n";
  ptr += "  var slider = document.getElementById(\"servoSlider\");\n";
  ptr += "  var servoP = document.getElementById(\"servoPos\");\n";
  ptr += "  servoP.innerHTML = slider.value;\n";
  ptr += "  slider.oninput = function() {\n";
  ptr += "    slider.value = this.value;\n";
  ptr += "    servoP.innerHTML = this.value;\n";
  ptr += "  }\n";
  ptr += "function servo() {\n";
  ptr += "  var xmlHttp = new XMLHttpRequest();\n";
  ptr += "  xmlHttp.open('GET', '/?value=' + this.value, false);\n";
  ptr += "  xmlHttp.send(null);\n";
  ptr += "  return xmlHttp.responseText;\n";
  ptr += "}\n";
  ptr += "document.getElementById('servoSlider').addEventListener('change', servo, false);\n";
  ptr += "</script>\n";

  ptr += "</body></html>";

  //GET /?value=180 HTTP/1.1
  if (server.arg("value") != "") {
    newDegree = myConstrain(server.arg("value").toInt(), servoDegreeMin, servoDegreeMax);

    bool rotated = false;
    byte attempts = 1;
    while (!rotated && attempts <= servoRotationMaxAttempts) {
      rotated = writeServo(newDegree);
      if (!rotated) {
        println("[FAIL] Servo rotation error! Attempt: " + String(attempts) + "/" + String(servoRotationMaxAttempts));
        attempts++;
        delay(100);
      }
    }
  }

  server.send(200, "text/html", ptr);
}

// Endpoint for getting status
void handleStatus() {
  JSONVar statusObj;

  statusObj["servoDegreeCurrent"] = newDegree;
  statusObj["servoDegreeMin"] = servoDegreeMin;
  statusObj["servoDegreeMax"] = servoDegreeMax;

  server.send(200, "application/json", JSON.stringify(statusObj));
}

// My constrain function
int myConstrain(int value, int minVal, int maxVal) {
  if (value < minVal) {
    value = minVal;
  }

  if (value > maxVal) {
    value = maxVal;
  }
  return value;
}

// Returns analog degree for servo
int getAnalogDegree() {
  int analogVal = analogRead(pinServoAnalog);
  analogVal = myConstrain(analogVal, servoAnalogMin, servoAnalogMax);
  analogDegree = map(analogVal, servoAnalogMin, servoAnalogMax, servoDegreeMin, servoDegreeMax);

  return analogDegree;
}

// Rotate servo
bool writeServo(int degree) {
  int start = getAnalogDegree();
  int end = degree;
  int position;

  if (start > end) {
    for (position = start; position >= end; position--)
    {
      servo.write(position);
      delay(servoDelay);
    }
  } else {
    for (position = start; position <= end; position++)
    {
      servo.write(position);
      delay(servoDelay);
    }
  }

  delay(100);
  position = getAnalogDegree();

  int checkMin = degree - servoAnalogDiff;
  int checkMax = degree + servoAnalogDiff;
  bool rotated = position > checkMin && position < checkMax;

  print(rotated ? "[OK] Servo is rotated!" : "[FAIL] Servo is not rotated!");
  print(" ");
  print(String(position) + "/" + String(degree) + " (Analog/Digital)");
  println(" | Diff min: " + String(checkMin) + " | Diff max: " + String(checkMax));

  return rotated;
}

// Wi-Fi initialization
void initWiFi() {
  WiFi.softAPConfig(local_IP, gateway, subnet);
  while (WiFi.softAP(ssid, password) != true) {
    delay(750);
  }

  println("AP SSID: " + String(ssid));
  println("AP IP: " + WiFi.softAPIP().toString());
}

// Servo initialization
void initServo() {
  pinMode(pinServoAnalog, INPUT);

  servo.attach(
    pinServo,
    servoMinUs,
    servoMaxUs
  );

  if (servo.attached()) {
    println("Servo attached");
  }

  writeServo(newDegree);
}

// Server initialization
void initServer() {
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.begin();

  println("HTTP server started");
}

// Print message to Serial
void print(String message) {
  if (DEBUG_MODE) {
    Serial.print(message);
  }
}

// Print message to Serial
void println(String message) {
  if (DEBUG_MODE) {
    Serial.println(message);
  }
}
