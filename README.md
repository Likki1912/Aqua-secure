# Aqua Secure — IoT Water-Level Monitoring

Aqua Secure is an ESP32-based prototype that measures tank water level with an ultrasonic sensor, displays the reading on an I2C LCD, triggers visual/audio alerts at a configured threshold, and sends readings to a web backend. It can also request SMS notifications through the CircuitDigest API.

## Hardware

- ESP32 development board
- Ultrasonic sensor (TRIG GPIO 27, ECHO GPIO 26)
- 16×2 I2C LCD (address `0x27`)
- Buzzer (GPIO 25)
- LED (GPIO 33)
- DFPlayer Mini and speaker (UART1: ESP32 RX 16, TX 17)

Check your actual board and wiring before powering the circuit. If using a 5 V ultrasonic sensor, do not connect its 5 V ECHO signal directly to an ESP32 GPIO; use a suitable voltage divider/level shifter.

## Software

- Arduino IDE
- ESP32 Arduino core
- ArduinoJson
- LiquidCrystal_I2C
- DFRobotDFPlayerMini

## Setup

1. Install the libraries listed above in Arduino IDE.
2. Copy `secrets.example.h` to a new file named `secrets.h`.
3. Fill in your own Wi-Fi, backend, phone number, SMS API key, and template ID in `secrets.h`.
4. Keep `secrets.h` private. It is ignored by Git.
5. Open `Aqua_Secure.ino`, select the correct ESP32 board and port, then compile and upload.
6. Add the audio file `0001.mp3` to the DFPlayer-compatible microSD card using the file naming convention required by the module.

## How it works

The ultrasonic sensor measures the distance to the water surface. The sketch calculates the level using `TANK_HEIGHT` and displays the percentage and distance on the LCD. When the level is greater than 90%, it activates the LED and pulsing buzzer, requests audio playback, and attempts an SMS notification. When Wi-Fi is available, it posts distance and level data to the configured backend.

## Calibration

Adjust `TANK_HEIGHT` and `MAX_DISTANCE` to match your installation. The current values are examples based on the supplied sketch; measure your tank and sensor mounting position before relying on the readings.

## Important notes

- This repository intentionally contains placeholders instead of live credentials.
- The backend URL and SMS API configuration must be supplied in `secrets.h`.
- SMS delivery depends on the provider account, API configuration, approved template, and network availability.
- Verify the backend API schema and response codes for your own deployment.
- The prototype is not a substitute for independent safety protections against overflow.

## Repository files

- `Aqua_Secure.ino` — ESP32 sketch
- `secrets.example.h` — safe configuration template
- `.gitignore` — excludes private `secrets.h`

## Author

G. Likhitha

## Hardware photo

![Aqua Secure hardware](images/aqua-secure-hardware.jpg)
