#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

PulseOximeter pox;

const int SAMPLE_COUNT = 5;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  if (!pox.begin()) {
    Serial.println("MAX30100 not found");
    while (1);
  }

  // Set LED current
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);

  Serial.println("Sensor warming up...");
  delay(3000);
  Serial.println("Place finger steadily");
}

void loop() {
  static int count = 0;
  static float hrSum = 0;
  static float spo2Sum = 0;

  pox.update();

  float hr = pox.getHeartRate();
  float spo2 = pox.getSpO2();

  // Accept only realistic values
  if (hr >= 70 && hr <= 120 && spo2 > 80 && spo2 <= 100) {
    hrSum += hr;
    spo2Sum += spo2;
    count++;

    // sampling delay kept exactly same
    delay(300);
  }

  if (count >= SAMPLE_COUNT) {
    Serial.print("BPM  : ");
    Serial.println(hrSum / SAMPLE_COUNT);

    Serial.print("SpO2 : ");
    Serial.println(spo2Sum / SAMPLE_COUNT);

    Serial.println("-------------------");

    count = 0;
    hrSum = 0;
    spo2Sum = 0;

    delay(10000);   // unchanged wait time
    Serial.println("Measuring again...");
  }
}
