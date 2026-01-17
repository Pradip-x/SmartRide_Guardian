#include <Wire.h>

#define MPU 0x68

// ===== Speaker settings =====
#define SPEAKER_PIN 25
#define CHANNEL 0
#define FREQUENCY 2000   // Hz
#define RESOLUTION 8    // 8-bit PWM

int16_t AccX, AccY, AccZ;
float Ax, Ay, Az;
float roll;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // ===== I2C setup =====
  Wire.begin(21, 22);   // SDA, SCL for ESP32
  Wire.setClock(400000);

  // Wake up MPU6050
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);   // Power management register
  Wire.write(0);     // Wake up sensor
  Wire.endTransmission(true);

  // ===== Speaker setup =====
  ledcAttach(SPEAKER_PIN, FREQUENCY, RESOLUTION);
  ledcWrite(SPEAKER_PIN, 0);   // start silent

  Serial.println("MPU6050 Ready");
}

void loop() {

  // ===== Read accelerometer data =====
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);   // ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);

  AccX = Wire.read() << 8 | Wire.read();
  AccY = Wire.read() << 8 | Wire.read();
  AccZ = Wire.read() << 8 | Wire.read();

  // Convert raw data to g
  Ax = AccX / 16384.0;
  Ay = AccY / 16384.0;
  Az = AccZ / 16384.0;

  // Roll calculation
  roll = atan2(Ay, Az) * 180.0 / PI;

  // ===== Roll direction output =====
  if (roll > 0) {
    Serial.print("Positive Roll: ");
  } else if (roll < 0) {
    Serial.print("Negative Roll: ");
  } else {
    Serial.print("No Roll: ");
  }

  Serial.print(roll, 2);
  Serial.println(" degrees");

  // ===== Beep logic =====
  if (roll > 25 || roll < -25) {
    // continuous beep
    ledcWrite(SPEAKER_PIN, 200);  // volume (0–255)
  } else {
    // silence
    ledcWrite(SPEAKER_PIN, 0);
  }

  delay(1000);   // 1 second delay
}
