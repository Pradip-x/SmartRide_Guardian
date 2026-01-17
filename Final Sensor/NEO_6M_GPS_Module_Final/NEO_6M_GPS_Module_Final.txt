#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

#define SPEAKER_PIN 25
#define CHANNEL 0
#define FREQUENCY 2000   // Hz
#define RESOLUTION 8    // 8-bit PWM

TinyGPSPlus gps;
HardwareSerial GPS(2);   // UART2

unsigned long lastPrint = 0;
unsigned long sameLocationStart = 0;

double lastLat = 0.0;
double lastLng = 0.0;

bool firstFix = true;
bool beeping = false;

void setup() {
  Serial.begin(115200);

  // GPS RX = 16, TX = 17
  GPS.begin(9600, SERIAL_8N1, 16, 17);

  ledcAttach(SPEAKER_PIN, FREQUENCY, RESOLUTION);
  ledcWrite(SPEAKER_PIN, 0);   // speaker OFF initially

  Serial.println("GPS Started...");
}

void loop() {
  while (GPS.available()) {
    gps.encode(GPS.read());
  }

  if (millis() - lastPrint >= 7000) {   // 7 seconds
    lastPrint = millis();

    if (gps.location.isValid()) {
      double currentLat = gps.location.lat();
      double currentLng = gps.location.lng();

      Serial.print("Latitude: ");
      Serial.println(currentLat, 6);
      Serial.print("Longitude: ");
      Serial.println(currentLng, 6);

      if (firstFix) {
        lastLat = currentLat;
        lastLng = currentLng;
        sameLocationStart = millis();
        firstFix = false;
      } else {
        if (currentLat == lastLat && currentLng == lastLng) {
          if (millis() - sameLocationStart >= 60000) { // 60 seconds
            ledcWrite(SPEAKER_PIN, 200); // continuous beep
            beeping = true;
          }
        } else {
          lastLat = currentLat;
          lastLng = currentLng;
          sameLocationStart = millis();

          ledcWrite(SPEAKER_PIN, 0); // stop beep
          beeping = false;
        }
      }
    } else {
      Serial.println("Waiting for GPS fix...");
    }

    Serial.println("--------------------");
  }
}
