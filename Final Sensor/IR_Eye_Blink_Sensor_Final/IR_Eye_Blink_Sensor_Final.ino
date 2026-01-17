#define EYE_SENSOR_PIN 27   // Connect OUT pin here

#define SPEAKER_PIN 25
#define CHANNEL 0
#define FREQUENCY 2000   // Hz
#define RESOLUTION 8     // 8-bit PWM

unsigned long eyeClosedStartTime = 0;
bool isBeeping = false;

void setup() {
  Serial.begin(115200);
  pinMode(EYE_SENSOR_PIN, INPUT);

  ledcAttach(SPEAKER_PIN, FREQUENCY, RESOLUTION);
  ledcWrite(SPEAKER_PIN, 0); // speaker OFF initially
}

void loop() {
  int sensorState = digitalRead(EYE_SENSOR_PIN);

  if (sensorState == LOW) {
    // Eye Closed
    Serial.println("Eye Closed");

    if (eyeClosedStartTime == 0) {
      eyeClosedStartTime = millis();
    }

    if (millis() - eyeClosedStartTime >= 10000 && !isBeeping) {
      ledcWrite(SPEAKER_PIN, 200); // beep ON
      isBeeping = true;
    }

  } else {
    // Eye Open
    Serial.println("Eye Open");

    eyeClosedStartTime = 0;

    if (isBeeping) {
      ledcWrite(SPEAKER_PIN, 0); // beep OFF
      isBeeping = false;
    }
  }

  delay(2000); // 2 seconds
}
