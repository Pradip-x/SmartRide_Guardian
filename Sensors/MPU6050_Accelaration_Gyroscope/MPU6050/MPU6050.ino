#include <Wire.h>

#define MPU 0x68

int16_t AccX, AccY, AccZ;
float Ax, Ay, Az;
float roll;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(21, 22);   // SDA, SCL for ESP32
  Wire.setClock(400000);

  // Wake up MPU6050
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);   // Power management register
  Wire.write(0);     // Wake up sensor
  Wire.endTransmission(true);

  Serial.println("MPU6050 Ready");
}

void loop() {

  // Read accelerometer data
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

  // Output only roll direction
  if (roll > 0) {
    Serial.print("Positive Roll: ");
  } else if (roll < 0) {
    Serial.print("Negative Roll: ");
  } else {
    Serial.print("No Roll: ");
  }

  Serial.print(roll, 2);
  Serial.println(" degrees");

  delay(1000);   // 1 seconds delay
}
