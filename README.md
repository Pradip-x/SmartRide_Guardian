#<h1>🏍️ Smart Helmet – ESP32 Sensor Connections Guide</h1>

This document explains how to connect all sensors and modules to ESP32 (WROOM-32) step by step.
It is written for beginners and focuses on correct wiring, safety, and stable working.


----

🔧 Components Used

ESP32 WROOM-32 (WiFi + Bluetooth)

MPU6050 (Gyroscope + Accelerometer)

MAX30100 (Heart Rate & SpO₂ Sensor)

MQ-135 (Air Quality Sensor)

IR Eye Blink Sensor

NEO-6M GPS Module

PAM8403 Audio Amplifier

Speaker (4Ω / 8Ω)

18650 Li-ion Battery

TP4056 Charging Module

Boost Converter (MT3608)

Connecting wires & resistors



---

⚡ Power Supply (Very Important)

Battery & Charging

18650 Battery → TP4056 (B+ and B−)

TP4056 OUT+ / OUT− → Boost Converter input


Boost Converter

Set output to 5V

This 5V line powers:

ESP32 (VIN)

PAM8403

MQ-135

GPS Module



ESP32 Power

VIN → 5V

GND → Common Ground


⚠️ Do NOT power ESP32 directly from battery.


---

🔌 Common Ground Rule

All modules must share the same GND:

ESP32 GND

Sensors GND

Amplifier GND

GPS GND



---

📡 I2C Sensors (MPU6050 + MAX30100)

Both sensors share the same I2C bus.

ESP32 I2C Pins

GPIO 21 → SDA

GPIO 22 → SCL



---

MPU6050 Connections

MPU6050 Pin	ESP32 Pin

VCC	3.3V
GND	GND
SDA	GPIO 21
SCL	GPIO 22
INT	Not Connected



---

MAX30100 Connections

MAX30100 Pin	ESP32 Pin

VCC	3.3V
GND	GND
SDA	GPIO 21
SCL	GPIO 22
INT	Not Connected


✅ Both sensors work safely at 3.3V logic.


---

🌫️ MQ-135 Air Quality Sensor

Power

VCC → 5V

GND → GND


Analog Output (Important Protection)

MQ-135 gives up to 5V output, but ESP32 ADC supports only 3.3V.

Use a voltage divider:

MQ-135 AO → 20kΩ → GPIO 34
                      |
                    10kΩ
                      |
                     GND

ESP32 Pin

GPIO 34 (ADC input)



---

👁️ IR Eye Blink Sensor

Connections

IR Sensor Pin	ESP32 Pin

VCC	3.3V
GND	GND
OUT	GPIO 27 (via 10kΩ resistor)


The resistor protects ESP32 from high voltage.


---

🛰️ GPS NEO-6M Module

Power

VCC → 5V

GND → GND


UART Communication (Serial2)

GPS Pin	ESP32 Pin

TX	GPIO 16 (with 10k–20k divider)
RX	GPIO 17


ESP32 UART2:

RX2 = GPIO 16

TX2 = GPIO 17



---

🔊 Speaker & PAM8403 Amplifier

PAM8403 Power

VCC → 5V

GND → GND

Add 470µF capacitor across VCC & GND


Audio Input

ESP32 GPIO 25 (DAC) → PAM8403 L-IN

PAM8403 R-IN → GND (mono)


Speaker

Speaker + → PAM8403 SPK+

Speaker − → PAM8403 SPK−



---

📌 ESP32 Pin Summary

Function	ESP32 GPIO

I2C SDA	21
I2C SCL	22
MQ-135 AQI	34
IR Eye Blink	27
GPS RX	16
GPS TX	17
Audio DAC	25
Power Input	VIN



---

✅ What This Setup Can Do

Detect rider head movement (MPU6050)

Measure heart rate & oxygen (MAX30100)

Detect drowsiness (IR Eye Sensor)

Monitor air quality (MQ-135)

Track location (GPS)

Play alerts & voice prompts (Speaker)

Support future Bluetooth / calls



---

⚠️ Safety Notes

Never connect 5V signals directly to ESP32 GPIO

Always use common ground

Use decoupling capacitors near sensors

Secure wiring for helmet vibration



----
THANKYOU
