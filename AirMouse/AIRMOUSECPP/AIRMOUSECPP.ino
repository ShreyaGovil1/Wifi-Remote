#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <Wire.h>
#include <ArduinoJson.h>

// ================== CONFIG ==================
const char* ssid = "XXXX";
const char* password = "XXXX";
const char* serverIP = "XXXX";
const int serverPort = 8888;

// Hardware pins
const int SDA_PIN = 4, SCL_PIN = 5, BUTTON_PIN = 0;
const int MPU_ADDR = 0x68;

// State variables
bool airMouseActive = false, isCalibrated = false;
bool lastButtonState = HIGH;
unsigned long buttonPressStart = 0;
bool wsConnected = false;

// Calibration offsets
float gyroXOffset = 0, gyroYOffset = 0, gyroZOffset = 0;

// Pointer movement settings (gyroscope-based)
const float sensitivity = 5.0f;        // Movement sensitivity
const float deadZone = 300.0f;         // Ignore small rotations
const int maxSpeed = 20;               // Maximum cursor speed
const float smoothing = 0.7f;          // Movement smoothing

// Smoothing variables
float smoothX = 0, smoothY = 0;

WebSocketsClient webSocket;
unsigned long lastMovement = 0;
unsigned long lastReconnect = 0;
const int moveInterval = 40; // ~25 FPS for smooth movement
const int reconnectInterval = 3000;

void setup() {
  Serial.begin(115200);
  Serial.println("\n🖱️ AIR MOUSE (Joystick Mode)");
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  // Initialize I2C and MPU6050
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  
  if (!initMPU6050()) {
    Serial.println("❌ MPU6050 failed!");
    while(1) { digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); delay(200); }
  }
  
  connectWiFi();
  
  // Setup WebSocket
  webSocket.begin(serverIP, serverPort, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  
  Serial.println("🎮 Controls:");
  Serial.println("  Short press → Toggle ON/OFF");
  Serial.println("  Long press  → Calibrate");
  Serial.println("  Very long   → Left click");
  Serial.println("  Move device in air → Cursor follows like a pointer!");
  Serial.println("  Keep steady → Cursor stops");
  
  blinkLED(3, 100);
}

void loop() {
  webSocket.loop();
  
  // Handle reconnection
  if (!wsConnected && millis() - lastReconnect > reconnectInterval) {
    Serial.println("🔄 Reconnecting...");
    webSocket.disconnect();
    webSocket.begin(serverIP, serverPort, "/");
    lastReconnect = millis();
  }
  
  handleButton();
  
  if (airMouseActive && isCalibrated && wsConnected && 
      millis() - lastMovement > moveInterval) {
    handleJoystickMovement();
    lastMovement = millis();
  }
  
  delay(10);
}

void connectWiFi() {
  Serial.print("📶 Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\n✅ Connected! IP: " + WiFi.localIP().toString());
}

bool initMPU6050() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); Wire.write(0x00); // Wake up MPU6050
  if (Wire.endTransmission() != 0) return false;
  
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C); Wire.write(0x00); // ±2g accelerometer range
  Wire.endTransmission();
  
  delay(100);
  return true;
}

void handleButton() {
  bool currentState = digitalRead(BUTTON_PIN);
  
  // Button pressed (falling edge)
  if (currentState == LOW && lastButtonState == HIGH) {
    buttonPressStart = millis();
  }
  
  // Button released (rising edge)
  if (currentState == HIGH && lastButtonState == LOW) {
    unsigned long duration = millis() - buttonPressStart;
    
    if (duration < 500) {
      // Short press - toggle air mouse
      if (isCalibrated) {
        airMouseActive = !airMouseActive;
        Serial.println(airMouseActive ? "🎮 AIR MOUSE ON" : "🛑 AIR MOUSE OFF");
        blinkLED(airMouseActive ? 2 : 1, 100);
      } else {
        Serial.println("❌ Calibrate first!");
      }
    } else if (duration < 3000) {
      // Long press - calibrate
      calibrateSensor();
    } else {
      // Very long press - left click
      sendCommand("leftclick");
    }
  }
  
  lastButtonState = currentState;
}

void calibrateSensor() {
  Serial.println("🎯 Calibrating...");
  blinkLED(5, 50);
  
  long gx = 0, gy = 0, gz = 0;
  const int samples = 150;
  
  for (int i = 0; i < samples; i++) {
    int16_t gyroX, gyroY, gyroZ;
    readGyro(gyroX, gyroY, gyroZ);
    gx += gyroX; gy += gyroY; gz += gyroZ;
    delay(10);
  }
  
  gyroXOffset = gx / samples;
  gyroYOffset = gy / samples;
  gyroZOffset = gz / samples;
  
  // Reset smoothing
  smoothX = 0;
  smoothY = 0;
  
  isCalibrated = true;
  Serial.printf("✅ Calibrated! Gyro Offsets: X:%.0f Y:%.0f Z:%.0f\n", 
                gyroXOffset, gyroYOffset, gyroZOffset);
  Serial.println("🖱️ Move device smoothly in air - cursor will follow!");
  blinkLED(3, 150);
}

void handleJoystickMovement() {
  int16_t gyroX, gyroY, gyroZ;
  readGyro(gyroX, gyroY, gyroZ);
  
  // Apply calibration and convert to movement
  float gx = (gyroX - gyroXOffset);
  float gy = (gyroY - gyroYOffset);
  
  // Apply dead zone
  if (abs(gx) < deadZone) gx = 0;
  if (abs(gy) < deadZone) gy = 0;
  
  // Apply smoothing
  smoothX = smoothing * smoothX + (1.0f - smoothing) * gx;
  smoothY = smoothing * smoothY + (1.0f - smoothing) * gy;
  
  // Convert to mouse movement (pointer style)
  int mouseX = constrain((int)(smoothY * sensitivity / 100.0f), -maxSpeed, maxSpeed);
  int mouseY = constrain((int)(-smoothX * sensitivity / 100.0f), -maxSpeed, maxSpeed);
  
  // Send movement if significant
  if (abs(mouseX) > 1 || abs(mouseY) > 1) {
    sendMouseMove(mouseX, mouseY);
  }
}

void readAccel(int16_t &x, int16_t &y, int16_t &z) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)MPU_ADDR, (size_t)6, true);
  
  x = (Wire.read() << 8) | Wire.read();
  y = (Wire.read() << 8) | Wire.read();
  z = (Wire.read() << 8) | Wire.read();
}

void sendCommand(String command) {
  if (wsConnected) {
    DynamicJsonDocument doc(128);
    doc["type"] = "command";
    doc["command"] = command;
    
    String payload;
    serializeJson(doc, payload);
    webSocket.sendTXT(payload);
    Serial.println("📤 " + command);
  } else {
    Serial.println("❌ Not connected - " + command + " failed");
  }
}

void sendMouseMove(int deltaX, int deltaY) {
  if (wsConnected) {
    DynamicJsonDocument doc(128);
    doc["type"] = "move";
    doc["deltaX"] = deltaX;
    doc["deltaY"] = deltaY;
    
    String payload;
    serializeJson(doc, payload);
    webSocket.sendTXT(payload);
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("🔌 WebSocket Disconnected");
      wsConnected = false;
      digitalWrite(LED_BUILTIN, HIGH); // LED off when disconnected
      break;
      
    case WStype_CONNECTED:
      {
        Serial.printf("✅ WebSocket Connected to: %s\n", payload);
        wsConnected = true;
        digitalWrite(LED_BUILTIN, LOW); // LED on when connected
        
        // Send connection confirmation
        DynamicJsonDocument doc(64);
        doc["type"] = "status";
        doc["device"] = "NodeMCU_AirMouse";
        String msg;
        serializeJson(doc, msg);
        webSocket.sendTXT(msg);
      }
      break;
      
    case WStype_TEXT:
      Serial.printf("📨 Received: %s\n", payload);
      break;
      
    case WStype_ERROR:
      Serial.printf("❌ WebSocket Error: %s\n", payload);
      wsConnected = false;
      break;
      
    default:
      break;
  }
}

void blinkLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_BUILTIN, LOW); delay(delayMs);
    digitalWrite(LED_BUILTIN, HIGH); delay(delayMs);
  }
}
void readGyro(int16_t &x, int16_t &y, int16_t &z) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x43); // Starting register for Gyro data
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)6, (bool)true);

  x = (Wire.read() << 8) | Wire.read();  // Gyro X
  y = (Wire.read() << 8) | Wire.read();  // Gyro Y
  z = (Wire.read() << 8) | Wire.read();  // Gyro Z
}
