#define EYE_SENSOR_PIN 27   // Connect OUT pin here

void setup() {
  Serial.begin(115200);
  pinMode(EYE_SENSOR_PIN, INPUT);
}

void loop() {
  int sensorState = digitalRead(EYE_SENSOR_PIN);

  if (sensorState == LOW) {
    // Most IR modules give LOW when object is detected
    Serial.println("Eye Closed");
  } else {
    Serial.println("Eye Open");
  }

  delay(2000); // 2 seconds
}
