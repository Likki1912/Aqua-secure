#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DFRobotDFPlayerMini.h>

// Aqua Secure - ESP32 water-level monitoring
// IMPORTANT: Keep real credentials in secrets.h, which must not be uploaded.

#include "secrets.h"

// Tank configuration (calibrate for your actual tank)
const float TANK_HEIGHT = 60.0;
const float MAX_DISTANCE = 55.0;

// Ultrasonic pins
const int TRIG_PIN = 27;
const int ECHO_PIN = 26;

// Buzzer and LED
const int BUZZER_PIN = 25;
const int LED_PIN = 33;

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// DFPlayer Mini on UART1 (RX=16, TX=17)
HardwareSerial mp3Serial(1);
DFRobotDFPlayerMini dfPlayer;
bool dfPlayerReady = false;

unsigned long lastBeep = 0;
unsigned long lastAudio = 0;
unsigned long lastSMS = 0;
bool buzzerState = false;
bool alertWasActive = false;

float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1.0; // timeout / invalid reading
  return duration * 0.0343 / 2.0;
}

void sendSMS_CircuitDigest(float level) {
  if (millis() - lastSMS < 15000) return;
  if (WiFi.status() != WL_CONNECTED) return;

  lastSMS = millis();
  String url = String("https://www.circuitdigest.cloud/send_sms?ID=") + CD_MSG_ID;

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", CD_API_KEY);

  String payload = String("{\"mobiles\":\"") + CD_PHONE +
                   "\",\"var1\":\"" + String(level, 1) +
                   "%\",\"var2\":\"HIGH\"}";

  int code = http.POST(payload);
  Serial.printf("SMS API response: %d\n", code);
  http.end();
}

void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Aqua Secure");
  delay(1000);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  mp3Serial.begin(9600, SERIAL_8N1, 16, 17);
  dfPlayerReady = dfPlayer.begin(mp3Serial);
  if (dfPlayerReady) {
    dfPlayer.volume(25);
    Serial.println("DFPlayer ready");
  } else {
    Serial.println("DFPlayer not detected");
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 20000) {
    delay(400);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? "\nWi-Fi connected" : "\nWi-Fi not connected; continuing offline");
}

void loop() {
  float distance = getDistance();

  if (distance < 2.0 || distance > MAX_DISTANCE) {
    Serial.println("Invalid ultrasonic reading");
    delay(500);
    return;
  }

  float waterHeight = TANK_HEIGHT - distance;
  float level = constrain((waterHeight / TANK_HEIGHT) * 100.0, 0.0, 100.0);

  Serial.printf("Distance: %.1f cm | Water level: %.1f%%\n", distance, level);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Water: ");
  lcd.print(level, 1);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("Dist: ");
  lcd.print(distance, 1);
  lcd.print("cm");

  bool alertActive = level > 90.0;
  if (alertActive) {
    digitalWrite(LED_PIN, HIGH);

    if (millis() - lastBeep >= 1000) {
      buzzerState = !buzzerState;
      digitalWrite(BUZZER_PIN, buzzerState);
      lastBeep = millis();
    }

    if (dfPlayerReady && millis() - lastAudio >= 8000) {
      dfPlayer.play(1); // SD card file commonly named 0001.mp3
      lastAudio = millis();
      Serial.println("Audio alert requested");
    }

    // Send on alert entry, then retry at most once per 15 seconds.
    if (!alertWasActive || millis() - lastSMS >= 15000) {
      sendSMS_CircuitDigest(level);
    }
  } else {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    buzzerState = false;
  }
  alertWasActive = alertActive;

  if (WiFi.status() == WL_CONNECTED) {
    StaticJsonDocument<200> doc;
    doc["mobileNumber"] = APP_MOBILE_NUMBER;
    doc["distance"] = distance;
    doc["levelPercentage"] = level;

    String json;
    serializeJson(doc, json);

    HTTPClient http;
    http.begin(WATER_LEVEL_SERVER_URL);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(json);
    Serial.printf("Backend POST response: %d\n", code);
    http.end();
  }

  delay(500);
}
