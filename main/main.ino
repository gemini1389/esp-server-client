#include <Servo.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Arduino_JSON.h>

// Print messages to Serial
#define DEBUG_MODE true

// GPIO the servo is attached to
#define pinServo        D4
#define pinServoAnalog  A0

// Config for servo
#define servoDelay        8
#define servoDiagDelay    30
#define servoMinUs        720  // 0
#define servoMaxUs        2400 // 180
#define servoMaxAttempts  3

// Min and max degree for rotation servo
#define servoDegreeMin  45  // 0
#define servoDegreeMax  105 // 170

// Min and max analog values
#define servoAnalogMin  348 // 221
#define servoAnalogMax  493 // 665

// Possible difference between values
#define servoAnalogDegreesDiff  5
#define servoAnalogAnalogDiff   50

// Credentials for Wi-Fi
const char* ssid     = "AGS-WiFi";
const char* password = "qwepoi123654789";

// Config for network
IPAddress local_IP(192, 168, 1, 13);
IPAddress gateway(192, 168, 1, 13);
IPAddress subnet(255, 255, 255, 0);

int newDegree = servoDegreeMin;
int analogDegree;
unsigned long timer;

Servo servo;
ESP8266WebServer server(80);

void setup() {
  delay(2000);

  if (DEBUG_MODE) {
    Serial.begin(115200);
  }
  println("[INIT] Debug mode is enabled");
  println("[INIT] Starting...");

  initWiFi();
  println("--------------");
  initServer();
  println("--------------");
  initServo();
  println("--------------");
  servoDiagnostics();

  println("[INIT] Initialization is complete");
}

void loop() {
  server.handleClient();
}

// Rotate servo
bool writeServo(int degree, bool isDiagnostics = false) {
  int start = getAnalogDegree();
  int end = degree;
  int position;

  if (start > end) {
    for (position = start; position >= end; position--)
    {
      servo.write(position);
      delay(isDiagnostics ? servoDiagDelay : servoDelay);
    }
  } else {
    for (position = start; position <= end; position++)
    {
      servo.write(position);
      delay(isDiagnostics ? servoDiagDelay : servoDelay);
    }
  }

  delay(100);

  return isServoRotated(degree);
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
    while (!rotated && attempts <= servoMaxAttempts) {
      rotated = writeServo(newDegree);
      if (!rotated) {
        println("[FAIL] Servo rotation error! Attempt: " + String(attempts) + "/" + String(servoMaxAttempts));
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

// It returns the status of whether the servo turned
bool isServoRotated(int degreeNeedle) {
  bool rotated = false;

  // Check by analog value
  int analogVal = analogRead(pinServoAnalog);
  int checkMinAnalog = servoAnalogMin - servoAnalogAnalogDiff;
  int checkMaxAnalog = servoAnalogMax + servoAnalogAnalogDiff;
  rotated = analogVal > checkMinAnalog && analogVal < checkMaxAnalog;

  print("[INFO] Analog: ");
  print(String(analogVal));
  println(" | Diff min: " + String(checkMinAnalog) + " | Diff max: " + String(checkMaxAnalog));

  // Check by digital value
  if (rotated) {
    int degreeAnalog = getAnalogDegree();
    int checkMin = degreeNeedle - servoAnalogDegreesDiff;
    int checkMax = degreeNeedle + servoAnalogDegreesDiff;
    rotated = degreeAnalog > checkMin && degreeAnalog < checkMax;

    print("[INFO] Digital: ");
    print(String(degreeAnalog) + "/" + String(degreeNeedle) + " (Analog/Digital)");
    println(" | Diff min: " + String(checkMin) + " | Diff max: " + String(checkMax));
  }

  println(rotated ? "[OK] Servo is rotated!" : "[FAIL] Servo is not rotated!");

  return rotated;
}

// Wi-Fi initialization
void initWiFi() {
  WiFi.softAPConfig(local_IP, gateway, subnet);
  while (WiFi.softAP(ssid, password) != true) {
    delay(750);
  }

  println("[INIT] AP SSID: " + String(ssid));
  println("[INIT] AP IP: " + WiFi.softAPIP().toString());
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
    println("[INIT] Servo attached");
  }

  writeServo(newDegree);
}

// Server initialization
void initServer() {
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.begin();

  println("[INIT] HTTP server started");
}

// Diagnostics for servo
void servoDiagnostics() {
  println("[INIT] Servo diagnostics: start");

  bool diagStatus = true;
  diagStatus = writeServo(servoDegreeMax, true);

  print(diagStatus ? "[OK]" : "[FAIL]");
  println(" Servo diagnostics #1 (to " + String(servoDegreeMax) + "°)");

  if (diagStatus) {
    diagStatus = writeServo(servoDegreeMin, true);
  }

  print(diagStatus ? "[OK]" : "[FAIL]");
  println(" Servo diagnostics #2 (to " + String(servoDegreeMin) + "°)");

  if (diagStatus) {
    diagStatus = writeServo(servoDegreeMax, true);
  }

  print(diagStatus ? "[OK]" : "[FAIL]");
  println(" Servo diagnostics #3 (to " + String(servoDegreeMax) + "°)");

  if (diagStatus) {
    diagStatus = writeServo(servoDegreeMin, true);
  }

  print(diagStatus ? "[OK]" : "[FAIL]");
  println(" Servo diagnostics #4 (to " + String(servoDegreeMin) + "°)");

  if (!diagStatus) {
    while (true) {
      if (millis() - timer >= 3000) {
        timer = millis();
        println("[FAIL] Servo diagnostics: Shit happens...");
      }
    }
  }

  println("[INIT] Servo diagnostics: success");
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
