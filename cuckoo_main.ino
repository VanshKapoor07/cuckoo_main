#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>


// Define motor pins
#define MOTOR1_IN1 14
#define MOTOR1_IN2 12
#define MOTOR2_IN1 27
#define MOTOR2_IN2 26
#define MOTOR1_EN 13  // Enable pin for Motor 1
#define MOTOR2_EN 25  // Enable pin for Motor 2

// Define motor states
#define FORWARD 1
#define BACKWARD 2
#define STOP 0


// Create a web server on port 80
WebServer server(80);

// Replace with your Node.js server IP
const char* nodeJSServerIP = "http://172.16.9.41:3003";

void setup() {
  // Start Serial Monitor
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
  pinMode(MOTOR2_IN1, OUTPUT);
  pinMode(MOTOR2_IN2, OUTPUT);
  pinMode(MOTOR1_EN, OUTPUT);
  pinMode(MOTOR2_EN, OUTPUT);


  // Create Access Point
  WiFi.softAP("ESP32_Motor_Control");
  Serial.println("Access Point created. IP address:");
  Serial.println(WiFi.softAPIP());



  // Setup web server routes
  server.on("/", handleRoot);


server.on("/forward", HTTP_GET, []() {
    setMotorState(FORWARD);
    
    server.send(200, "text/html", "<h1>Motor is moving forward</h1>");
});

  server.on("/backward", []() {
    setMotorState(BACKWARD);
    server.send(200, "text/html", "<h1>Motor is moving backward</h1>");
  });
  server.on("/stop", []() {
    setMotorState(STOP);
    server.send(200, "text/html", "<h1>Motor is stopped</h1>");
  });

  // Serve CSS and JS files
  server.on("/style.css", []() {
    File file = SPIFFS.open("/style.css", "r");
    if (!file) {
      server.send(404, "text/plain", "File Not Found");
      return;
    }
    server.streamFile(file, "text/css");
    file.close();
  });

  server.on("/main.js", []() {
    File file = SPIFFS.open("/main.js", "r");
    if (!file) {
      server.send(404, "text/plain", "File Not Found");
      return;
    }
    server.streamFile(file, "application/javascript");
    file.close();
  });

  server.on("/set_time", HTTP_POST, []() {
    float set_time = 0;
    if (server.hasArg("plain")) {
        String body = server.arg("plain");
        
        // Parse the JSON data
        StaticJsonDocument<200> doc;
        deserializeJson(doc, body);
        set_time = doc["set_time"];
        
        Serial.println("Set time interval: " + String(set_time) + " seconds");
        
        // Process the set_time value (e.g., set a timer)
    }
    server.send(200, "text/html", "<h1>Time updated successfully!</h1>");
    setMotorState(STOP);
    while (true){
    delay(set_time*1000);
    setMotorState(FORWARD);
    delay(1500);
    setMotorState(STOP);
    delay(1000);
    setMotorState(BACKWARD);
    delay(1500);
    setMotorState(STOP);
    
    }

});
  // Start the server
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  // Handle web server requests
  server.handleClient();
}

void handleRoot() {
  // Redirect to Node.js server's index page
  server.sendHeader("Location", nodeJSServerIP);
  server.send(302, "text/plain", "Redirecting to Node.js server...");
}

void setMotorState(int state) {
  switch(state) {
    case FORWARD:
      digitalWrite(MOTOR1_IN1, HIGH);
      digitalWrite(MOTOR1_IN2, LOW);
      digitalWrite(MOTOR2_IN1, HIGH);
      digitalWrite(MOTOR2_IN2, LOW);
      analogWrite(MOTOR1_EN, 200);
      analogWrite(MOTOR2_EN, 200);
      break;
    case BACKWARD:
      digitalWrite(MOTOR1_IN1, LOW);
      digitalWrite(MOTOR1_IN2, HIGH);
      digitalWrite(MOTOR2_IN1, LOW);
      digitalWrite(MOTOR2_IN2, HIGH);
      analogWrite(MOTOR1_EN, 200);
      analogWrite(MOTOR2_EN, 200);
      break;
    case STOP:
      digitalWrite(MOTOR1_IN1, LOW);
      digitalWrite(MOTOR1_IN2, LOW);
      digitalWrite(MOTOR2_IN1, LOW);
      digitalWrite(MOTOR2_IN2, LOW);
      analogWrite(MOTOR1_EN, 0);  // Stop the motor by setting speed to 0
      analogWrite(MOTOR2_EN, 0);  // Stop the motor by setting speed to 0
      break;
  }
}
