#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>

// Define motor pins
#define MOTOR1_IN1 14
#define MOTOR1_IN2 12
#define MOTOR2_IN1 27
#define MOTOR2_IN2 26

// Define motor states
#define FORWARD 1
#define BACKWARD 2
#define STOP 0


// Create a web server on port 80
WebServer server(80);

// Replace with your Node.js server IP
const char* nodeJSServerIP = "http://192.168.56.1:3003";

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

  server.on("/setTime", []() {
    if (server.hasArg("set_time")) {
      float set_time = server.arg("set_time").toFloat();  // Get the time value from the form
      Serial.println("Set time interval: " + String(set_time) + " seconds");

      // Send confirmation back to the client
      server.send(200, "text/html", R"(
        <h1>Time updated successfully!</h1>
        <button onclick="location.href='/'">Go back</button>
      )");

      // Reset the motor state and timer
//      previousMillis = millis();
//      isMotorRunning = true;
    } else {
      server.send(200, "text/html", "<h1>No time provided</h1><br><a href='/'>Go Back</a>");
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
      break;
    case BACKWARD:
      digitalWrite(MOTOR1_IN1, LOW);
      digitalWrite(MOTOR1_IN2, HIGH);
      digitalWrite(MOTOR2_IN1, LOW);
      digitalWrite(MOTOR2_IN2, HIGH);
      break;
    case STOP:
      digitalWrite(MOTOR1_IN1, LOW);
      digitalWrite(MOTOR1_IN2, LOW);
      digitalWrite(MOTOR2_IN1, LOW);
      digitalWrite(MOTOR2_IN2, LOW);
      break;
  }
}
