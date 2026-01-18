#include <Wire.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include "MAX30100_PulseOximeter.h"
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// ===== WiFi Credentials =====
const char* ssid = "Samsung Galaxy A56 5G";
const char* password = "Terimakichut";

// ===== Pin Definitions =====
#define EYE_SENSOR_PIN 39
#define SPEAKER_PIN 25
#define MQ135_PIN 34
#define MPU_ADDR 0x68

// ===== Speaker Settings =====
#define CHANNEL 0
#define FREQUENCY 2000
#define RESOLUTION 8

// ===== Objects =====
PulseOximeter pox;
TinyGPSPlus gps;
HardwareSerial GPS(2);
WebSocketsServer webSocket = WebSocketsServer(81);

// ===== Eye Blink Variables =====
unsigned long eyeClosedStartTime = 0;
bool eyeBeeping = false;

// ===== MPU6050 Variables =====
int16_t AccX, AccY, AccZ;
float roll = 0;
bool mpuBeeping = false;
unsigned long lastMPURead = 0;

// ===== MAX30100 Variables =====
const int SAMPLE_COUNT = 5;
int hrCount = 0;
float hrSum = 0;
float spo2Sum = 0;
float avgHR = 0;
float avgSpO2 = 0;
unsigned long lastHealthRead = 0;

// ===== GPS Variables =====
unsigned long lastGPSPrint = 0;
unsigned long sameLocationStart = 0;
double lastLat = 0.0;
double lastLng = 0.0;
double currentLat = 0.0;
double currentLng = 0.0;
bool firstFix = true;
bool gpsBeeping = false;

// ===== MQ135 Variables =====
unsigned long lastAirRead = 0;
int airQualityValue = 0;
String airQualityStatus = "Unknown";

// ===== WebSocket Update =====
unsigned long lastWSUpdate = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // ===== Pin Setup =====
  pinMode(EYE_SENSOR_PIN, INPUT);
  
  // ===== Speaker Setup =====
  ledcAttach(SPEAKER_PIN, FREQUENCY, RESOLUTION);
  ledcWrite(SPEAKER_PIN, 0);

  // ===== WiFi Setup =====
  Serial.println("\n[WiFi] Connecting...");
  WiFi.begin(ssid, password);
  WiFi.setSleep(false); // Disable WiFi sleep for better WebSocket performance
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Connected!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("[WiFi] WebSocket Server: ws://");
    Serial.print(WiFi.localIP());
    Serial.println(":81");
  } else {
    Serial.println("\n[WiFi] Connection failed - continuing without WiFi");
  }

  // ===== WebSocket Setup =====
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("[WebSocket] Server started on port 81");

  // ===== I2C Setup =====
  Wire.begin(21, 22);
  Wire.setClock(100000);

  // ===== MPU6050 Init =====
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  delay(100);

  // ===== MAX30100 Init =====
  if (!pox.begin()) {
    Serial.println("MAX30100 not found - continuing without it");
  } else {
    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    Serial.println("MAX30100 ready");
  }

  // ===== GPS Setup =====
  GPS.begin(9600, SERIAL_8N1, 16, 17);

  Serial.println("=== Driver Safety System Started ===");
  Serial.println("Sensors: Eye Blink | MPU6050 | MAX30100 | GPS | MQ-135");
  Serial.println("=====================================\n");
}

void loop() {
  unsigned long currentMillis = millis();

  // ===== WebSocket Handler =====
  webSocket.loop();

  // ===== Eye Blink Sensor (always check) =====
  checkEyeBlink();

  // ===== MPU6050 Reading (every 200ms for faster tilt detection) =====
  if (currentMillis - lastMPURead >= 200) {
    lastMPURead = currentMillis;
    readMPU6050();
  }

  // ===== MAX30100 Reading (every 1000ms, health data doesn't need to be as fast) =====
  if (currentMillis - lastHealthRead >= 1000) {
    lastHealthRead = currentMillis;
    readMAX30100();
  }

  // ===== GPS Reading (continuous) =====
  while (GPS.available()) {
    gps.encode(GPS.read());
  }
  
  if (currentMillis - lastGPSPrint >= 7000) {
    lastGPSPrint = currentMillis;
    checkGPS();
  }

  // ===== MQ-135 Air Quality (every 10s) =====
  if (currentMillis - lastAirRead >= 10000) {
    lastAirRead = currentMillis;
    readAirQuality();
  }

  // ===== Speaker Control (priority based) =====
  updateSpeaker();

  // ===== Send WebSocket Data (every 200ms, synced with MPU updates) =====
  if (currentMillis - lastWSUpdate >= 200) {
    lastWSUpdate = currentMillis;
    sendWebSocketData();
  }
}

void checkEyeBlink() {
  int sensorState = digitalRead(EYE_SENSOR_PIN);

  if (sensorState == LOW) {
    Serial.println("[EYE] Closed");
    
    if (eyeClosedStartTime == 0) {
      eyeClosedStartTime = millis();
    }

    if (millis() - eyeClosedStartTime >= 10000 && !eyeBeeping) {
      eyeBeeping = true;
      Serial.println("[ALERT] Eyes closed > 10s!");
    }
  } else {
    Serial.println("[EYE] Open");
    eyeClosedStartTime = 0;
    eyeBeeping = false;
  }
}

void readMPU6050() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);

  AccX = Wire.read() << 8 | Wire.read();
  AccY = Wire.read() << 8 | Wire.read();
  AccZ = Wire.read() << 8 | Wire.read();

  float Ax = AccX / 16384.0;
  float Ay = AccY / 16384.0;
  float Az = AccZ / 16384.0;

  roll = atan2(Ay, Az) * 180.0 / PI;

  Serial.print("[MPU] Roll: ");
  Serial.print(roll, 2);
  Serial.println(" deg");

  if (roll > 25 || roll < -25) {
    mpuBeeping = true;
    Serial.println("[ALERT] Excessive head tilt!");
  } else {
    mpuBeeping = false;
  }
}

void readMAX30100() {
  pox.update();

  float hr = pox.getHeartRate();
  float spo2 = pox.getSpO2();

  if (hr >= 70 && hr <= 120 && spo2 > 80 && spo2 <= 100) {
    hrSum += hr;
    spo2Sum += spo2;
    hrCount++;
  }

  if (hrCount >= SAMPLE_COUNT) {
    avgHR = hrSum / SAMPLE_COUNT;
    avgSpO2 = spo2Sum / SAMPLE_COUNT;
    
    Serial.print("[HEALTH] BPM: ");
    Serial.print(avgHR, 1);
    Serial.print(" | SpO2: ");
    Serial.print(avgSpO2, 1);
    Serial.println("%");

    hrCount = 0;
    hrSum = 0;
    spo2Sum = 0;
  }
}

void checkGPS() {
  if (gps.location.isValid()) {
    currentLat = gps.location.lat();
    currentLng = gps.location.lng();

    Serial.print("[GPS] Lat: ");
    Serial.print(currentLat, 6);
    Serial.print(" | Lng: ");
    Serial.println(currentLng, 6);

    if (firstFix) {
      lastLat = currentLat;
      lastLng = currentLng;
      sameLocationStart = millis();
      firstFix = false;
    } else {
      if (currentLat == lastLat && currentLng == lastLng) {
        if (millis() - sameLocationStart >= 60000) {
          gpsBeeping = true;
          Serial.println("[ALERT] Vehicle stationary > 60s!");
        }
      } else {
        lastLat = currentLat;
        lastLng = currentLng;
        sameLocationStart = millis();
        gpsBeeping = false;
      }
    }
  } else {
    Serial.println("[GPS] Waiting for fix...");
  }
}

void readAirQuality() {
  airQualityValue = analogRead(MQ135_PIN);

  if (airQualityValue < 800)
    airQualityStatus = "Excellent";
  else if (airQualityValue < 1300)
    airQualityStatus = "Good";
  else if (airQualityValue < 1800)
    airQualityStatus = "Moderate";
  else if (airQualityValue < 2300)
    airQualityStatus = "Bad";
  else if (airQualityValue < 2800)
    airQualityStatus = "Severe";
  else
    airQualityStatus = "Hazardous";

  Serial.print("[AIR] ADC: ");
  Serial.print(airQualityValue);
  Serial.print(" | Quality: ");
  Serial.println(airQualityStatus);
}

void updateSpeaker() {
  // Priority: Eye > MPU > GPS
  if (eyeBeeping || mpuBeeping || gpsBeeping) {
    ledcWrite(SPEAKER_PIN, 200);
  } else {
    ledcWrite(SPEAKER_PIN, 0);
  }
}

void sendWebSocketData() {
  StaticJsonDocument<384> doc;
  
  // Eye sensor
  doc["eye"]["status"] = eyeBeeping ? "CLOSED" : "OPEN";
  doc["eye"]["alert"] = eyeBeeping;
  
  // MPU6050
  doc["mpu"]["roll"] = roll;
  doc["mpu"]["alert"] = mpuBeeping;
  
  // MAX30100
  doc["health"]["heartRate"] = avgHR;
  doc["health"]["spO2"] = avgSpO2;
  
  // GPS
  doc["gps"]["latitude"] = currentLat;
  doc["gps"]["longitude"] = currentLng;
  doc["gps"]["valid"] = gps.location.isValid();
  doc["gps"]["alert"] = gpsBeeping;
  
  // Air Quality
  doc["air"]["value"] = airQualityValue;
  doc["air"]["status"] = airQualityStatus;
  
  // Overall alert status
  doc["alertActive"] = (eyeBeeping || mpuBeeping || gpsBeeping);
  
  String jsonString;
  serializeJson(doc, jsonString);
  webSocket.broadcastTXT(jsonString);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WebSocket] Client #%u disconnected\n", num);
      break;
      
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[WebSocket] Client #%u connected from %d.%d.%d.%d\n", 
                      num, ip[0], ip[1], ip[2], ip[3]);
      }
      break;
      
    case WStype_TEXT:
      Serial.printf("[WebSocket] Message from #%u: %s\n", num, payload);
      break;
  }
}