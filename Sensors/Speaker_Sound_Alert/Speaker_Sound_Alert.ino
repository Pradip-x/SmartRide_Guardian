#define SPEAKER_PIN 25
#define CHANNEL 0
#define FREQUENCY 2000   // Hz
#define RESOLUTION 8    // 8-bit PWM

void setup() {
  ledcAttach(SPEAKER_PIN, FREQUENCY, RESOLUTION);
  ledcWrite(SPEAKER_PIN, 200); // volume (0–255)
}

void loop() {
  // continuous beep
}
