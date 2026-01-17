#include <Wire.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

/* ================= COMMON DEFINES ================= */
#define SPEAKER_PIN 25
#define FREQUENCY 2000
#define RESOLUTION 8

/* ================= IR EYE BLINK ================= */
#define EYE_SENSOR_PIN 35
unsigned long eyeClosedStartTime = 0;
bool eyeBeeping = false;
unsigned long lastEyeCheck = 0;

/* ================= MPU6050 ================= */
#define MPU 0x68
int16_t AccX, AccY, AccZ;
float Ax, Ay, Az;
float roll;
unsigned long lastMPURead = 0;

/* ================= HR & SPO2 ================= */
float simulatedHR = 75;
float simulatedSpO2 = 97;
bool simulateRandom = true;
unsigned long lastHRUpdate = 0;

/* ================= GPS ================= */
TinyGPSPlus gps;
HardwareSerial GPS(2);
unsigned long lastGPSPrint = 0;

/* ================= MQ135 ================= */
#define MQ135_PIN 34
unsigned long lastMQRead = 0;

/* ================= WEBSOCKET ================= */
WebSocketsServer webSocket(81);

/* ================================================= */

void broadcastJSON(String msg) {
  webSocket.broadcastTXT(msg);
  Serial.println(msg);
}

void onWsEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    String msg = String((char*)payload);

    if (msg.indexOf("HR") >= 0) {
      int i = msg.indexOf("HR") + 4;
      simulatedHR = msg.substring(i).toFloat();
    }

    if (msg.indexOf("SPO2") >= 0) {
      int i = msg.indexOf("SPO2") + 6;
      simulatedSpO2 = msg.substring(i).toFloat();
    }

    if (msg.indexOf("MODE") >= 0) {
      simulateRandom = msg.indexOf("RANDOM") >= 0;
    }
  }
}

void setup() {
  Serial.begin(115200);

  /* I2C */
  Wire.begin(21, 22);
  Wire.setClock(400000);

  /* Speaker (ESP32 core v3 FIX) */
  ledcAttach(SPEAKER_PIN, FREQUENCY, RESOLUTION);
  ledcWrite(SPEAKER_PIN, 0);

  /* Eye Blink */
  pinMode(EYE_SENSOR_PIN, INPUT);

  /* MPU6050 */
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  Serial.println("MPU6050 Ready");

  /* GPS */
  GPS.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("GPS Started");

  /* WiFi AP */
  WiFi.softAP("ESP32-Data", "esp32pass");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.softAPIP());

  /* WebSocket */
  webSocket.begin();
  webSocket.onEvent(onWsEvent);
  Serial.println("WebSocket running on port 81");
}

void loop() {
  webSocket.loop();

  /* ---------- IR EYE BLINK ---------- */
  if (millis() - lastEyeCheck >= 2000) {
    lastEyeCheck = millis();
    int state = digitalRead(EYE_SENSOR_PIN);

    if (state == LOW) {
      if (eyeClosedStartTime == 0)
        eyeClosedStartTime = millis();

      if (millis() - eyeClosedStartTime >= 10000 && !eyeBeeping) {
        ledcWrite(SPEAKER_PIN, 200);
        eyeBeeping = true;
      }
    } else {
      eyeClosedStartTime = 0;
      ledcWrite(SPEAKER_PIN, 0);
      eyeBeeping = false;
    }
  }

  /* ---------- MPU6050 ---------- */
  if (millis() - lastMPURead >= 1000) {
    lastMPURead = millis();

    Wire.beginTransmission(MPU);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 6, true);

    AccX = Wire.read() << 8 | Wire.read();
    AccY = Wire.read() << 8 | Wire.read();
    AccZ = Wire.read() << 8 | Wire.read();

    Ax = AccX / 16384.0;
    Ay = AccY / 16384.0;
    Az = AccZ / 16384.0;

    roll = atan2(Ay, Az) * 180.0 / PI;
    broadcastJSON("{\"ROLL\":" + String(roll, 2) + "}");
  }

  /* ---------- SIMULATED HR / SPO2 ---------- */
  if (millis() - lastHRUpdate >= 300) {
    lastHRUpdate = millis();

    if (simulateRandom) {
      simulatedHR += random(-2, 3);
      simulatedSpO2 += random(-1, 2);
    }

    simulatedHR = constrain(simulatedHR, 60, 120);
    simulatedSpO2 = constrain(simulatedSpO2, 90, 100);

    broadcastJSON("{\"HR\":" + String(simulatedHR) +
                  ",\"SPO2\":" + String(simulatedSpO2) + "}");
  }

  /* ---------- GPS ---------- */
  while (GPS.available())
    gps.encode(GPS.read());

  if (millis() - lastGPSPrint >= 7000) {
    lastGPSPrint = millis();

    if (gps.location.isValid()) {
      broadcastJSON("{\"LAT\":" + String(gps.location.lat(), 6) +
                    ",\"LNG\":" + String(gps.location.lng(), 6) + "}");
    }
  }

  /* ---------- MQ135 ---------- */
  if (millis() - lastMQRead >= 10000) {
    lastMQRead = millis();
    int val = analogRead(MQ135_PIN);

    String quality;
    if (val < 800) quality = "Excellent";
    else if (val < 1300) quality = "Good";
    else if (val < 1800) quality = "Moderate";
    else if (val < 2300) quality = "Bad";
    else quality = "Hazardous";

    broadcastJSON("{\"AIR\":\"" + quality + "\",\"ADC\":" + String(val) + "}");
  }
}
