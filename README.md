## Web Server using ESP8622 with servo and client with OLED
The project consists of two parts:
- main ([scheme](main/main.jpg))
- client ([scheme](client/client.jpg))

**main** - the web server with servo. Has a web interface for browsers that allows you to control the servo. There is also an API-like endpoint that returns the servo status in JSON format.

**client** - a web client that connects to main via Wi-Fi, receives information about the status of the servo and displays it on the OLED display.

A fourth wire was connected to the servo. It is soldered
  to the second output of the potentiometer inside the servo and connected to A0 on the board. The values are not very accurate, but they are enough to understand whether the servo has turned to the desired angle or not.

This sketch is set to rotate the servo in the range of 45-105 degrees. Equivalent values were also found for analog polling of the servo.

### Interfaces

Web interface: http://192.168.1.13/

API endpoint: http://192.168.1.13/status

### Default settings
**Wi-Fi:**
```
const char* ssid     = "AGS-WiFi";
const char* password = "qwepoi123654789";
```

**Network**
```
IPAddress local_IP(192, 168, 1, 13);
IPAddress gateway(192, 168, 1, 13);
IPAddress subnet(255, 255, 255, 0);
```

**Servo**
```
#define servoDegreeMin  45  // 0
#define servoDegreeMax  105 // 170

#define servoAnalogMin  348 // 221
#define servoAnalogMax  493 // 665
```

### Libraries for Arduino IDE:
- [ESP8266 and ESP32 OLED driver for SSD1306 displays](https://github.com/ThingPulse/esp8266-oled-ssd1306)
- [Arduino_JSON](http://github.com/arduino-libraries/Arduino_JSON)
- [Servo](http://www.arduino.cc/en/Reference/Servo)

### Hardware for project:
- Lolin NodeMCU v3 (x2)
- Sparkfun S05NF (micro)
- 0.96" OLED Display Module 128X64 I2C
- Dupont wires
- Breadboard
- Micro USB Cable
- MB102 Breadboard Power Supply
- DC 9V 1A Power Supply Adapter 5.5x2.1mm
