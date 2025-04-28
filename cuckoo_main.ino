#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <time.h>
#include "HX711.h"

// Replace with your Wi-Fi credentials
const char* ssid = "moto g54 5G_1156";
const char* password = "12345678";

// Set time offset for IST (GMT+5:30)
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;

// Define motor pins
#define MOTOR1_IN1 12             
#define MOTOR1_IN2 14             
#define MOTOR1_EN 13  // Enable pin for Motor 1     

// HX711 Load Cell pins
#define DOUT 22
#define CLK 21

// Define motor states
#define FORWARD_CUCKOO 1
#define FORWARD_CUCKOO 1
#define BACKWARD_CUCKOO 2
#define STOP 0

// Initialize HX711
HX711 scale;
// Calibration factor (initially set to 1000.0, needs adjustment based on your load cell)
float calibration_factor = 1000.0;

WebServer server(80);

const char* nodeJSServerIP = "http://172.16.9.221:3003";

void setup() {
  Serial.begin(115200);
  
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");
  
  // Initialize motor pins
  pinMode(MOTOR1_IN1, OUTPUT);
  pinMode(MOTOR1_IN2, OUTPUT);
  pinMode(MOTOR1_EN, OUTPUT);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected. IP:");
  Serial.println(WiFi.localIP());

  // Set up NTP time
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");

  // Initialize the HX711 scale
  scale.begin(DOUT, CLK);
  if (scale.is_ready()) {
    Serial.println("HX711 is ready.");
  } else {
    Serial.println("HX711 not found.");
    // Continue even if HX711 is not found
  }
  
  Serial.println("Taring... Remove any weight from the scale.");
  delay(2000);
  scale.tare();
  Serial.println("Tare done.");
  scale.set_scale(calibration_factor); // Setting the initial calibration factor

  // Setup web server routes
  server.on("/", handleRoot);

  server.on("/forward", HTTP_GET, []() {
    setMotorState(FORWARD_CUCKOO);
    server.send(200, "text/html", "<h1>Motor is moving forward</h1>");
  });

  server.on("/backward", HTTP_GET, []() {
    setMotorState(BACKWARD_CUCKOO);
    server.send(200, "text/html", "<h1>Motor is moving backward</h1>");
  });

  server.on("/stop", HTTP_GET, []() {
    setMotorState(STOP);
    server.send(200, "text/html", "<h1>Motor is stopped</h1>");
  });

  // Route for getting weight data
  server.on("/weight", HTTP_GET, []() {
    if (scale.is_ready()) {
      float weight = scale.get_units(5); // Average of 5 readings
      
      StaticJsonDocument<200> doc;
      doc["weight"] = weight;
      doc["unit"] = "g";
      
      String jsonString;
      serializeJson(doc, jsonString);
      server.send(200, "application/json", jsonString);
    } else {
      server.send(503, "application/json", "{\"error\":\"HX711 not ready\"}");
    }
  });

  // Serve CSS and JS files
  server.on("/style.css", HTTP_GET, []() {
    File file = SPIFFS.open("/style.css", "r");
    if (!file) {
      server.send(404, "text/plain", "File Not Found");
      return;
    }
    server.streamFile(file, "text/css");
    file.close();
  });

  server.on("/main.js", HTTP_GET, []() {
    File file = SPIFFS.open("/main.js", "r");
    if (!file) {
      server.send(404, "text/plain", "File Not Found");
      return;
    }
    server.streamFile(file, "application/javascript");
    file.close();
  });
  
  // Route for time
  server.on("/get-time", HTTP_GET, []() {
    Serial.println("Received /get-time");
    time_t now = time(nullptr);
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to get time");
      server.send(500, "application/json", "{\"error\":\"Failed to get time\"}");
      return;
    }

    StaticJsonDocument<200> doc;
    char formattedTime[30];
    strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", &timeinfo);
    doc["time"] = formattedTime;

    String jsonString;
    serializeJson(doc, jsonString);
    server.send(200, "application/json", jsonString);
  });

  // Route for calibrating the scale
  server.on("/calibrate", HTTP_GET, []() {
    String factorStr = server.arg("factor");
    if (factorStr != "") {
      calibration_factor = factorStr.toFloat();
      scale.set_scale(calibration_factor);
      server.send(200, "application/json", "{\"success\":true,\"calibration_factor\":" + factorStr + "}");
    } else {
      server.send(400, "application/json", "{\"error\":\"Missing 'factor' parameter\"}");
    }
  });

  // Route for taring the scale
  server.on("/tare", HTTP_GET, []() {
    scale.tare();
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Scale tared successfully\"}");
  });

  server.begin();
  Serial.println("Web server started.");
  Serial.print("Access the server at: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();
  
  
  
  static unsigned long lastWeightPrint = 0;
  if (millis() - lastWeightPrint > 5000) { // Print every 5 seconds
    lastWeightPrint = millis();
    if (scale.is_ready()) {
      float weight = scale.get_units(5);
      Serial.print("Weight: ");
      Serial.print(weight, 2); // 2 decimal places
      Serial.println(" g");
    }
  }
}

void handleRoot() {
  // Redirect to Node.js server's index page
  server.sendHeader("Location", nodeJSServerIP);
  server.send(302, "text/plain", "Redirecting to Node.js server...");
}

void setMotorState(int state) {
  switch (state) {
    case FORWARD_CUCKOO:
    case FORWARD_CUCKOO:
      digitalWrite(MOTOR1_IN1, HIGH);
      digitalWrite(MOTOR1_IN2, LOW);
      analogWrite(MOTOR1_EN, 100); 
      break;
   
    case BACKWARD_CUCKOO:
      digitalWrite(MOTOR1_IN1, LOW);
      digitalWrite(MOTOR1_IN2, HIGH);
      analogWrite(MOTOR1_EN, 100);
      analogWrite(MOTOR1_EN, 100);
      break;
    
    case STOP:
      digitalWrite(MOTOR1_IN1, LOW);
      digitalWrite(MOTOR1_IN2, LOW);
      analogWrite(MOTOR1_EN, 0);  // Stop the motor by setting speed to 0
      break;
  }
}
