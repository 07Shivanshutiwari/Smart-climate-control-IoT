/*
 * ============================================================
 *  Personalized Smart Ambient Climate Control System
 *  Hardware: NodeMCU ESP8266 (or Arduino Uno + ESP8266 module)
 *  Author:   [Your Name]
 *  Version:  2.0 (with Web Dashboard + IoT)
 * ============================================================
 *
 *  FEATURES:
 *  - Auto fan speed control based on ambient temperature
 *  - Buzzer alarm when temperature exceeds 60°C
 *  - Cold LED indicator when temperature drops below 18°C
 *  - LDR (Light Dependent Resistor) light sensing
 *  - WiFi connectivity (ESP8266)
 *  - Remote fan speed override via Web Dashboard
 *  - Real-time data push to Node.js server
 *
 *  PIN CONNECTIONS:
 *  DHT11       → D4 (GPIO2)
 *  Fan (PWM)   → D1 (GPIO5)  [via transistor/MOSFET]
 *  LDR         → A0
 *  Buzzer      → D5 (GPIO14)
 *  Cold LED    → D6 (GPIO12)
 * ============================================================
 */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

// ─── WiFi Credentials ──────────────────────────────────────
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ─── Server Configuration ──────────────────────────────────
const char* SERVER_URL    = "http://YOUR_SERVER_IP:3000";

// ─── Pin Definitions ───────────────────────────────────────
#define DHT_PIN        D4    // DHT11 data pin
#define DHT_TYPE       DHT11
#define FAN_PIN        D1    // PWM output to fan (via transistor)
#define LDR_PIN        A0    // LDR analog input
#define BUZZER_PIN     D5    // Active buzzer
#define COLD_LED_PIN   D6    // Cold temperature LED indicator

// ─── Temperature Thresholds ────────────────────────────────
#define TEMP_ALARM_HIGH  60.0f   // Buzzer triggers above this (°C)
#define TEMP_COLD_LOW    18.0f   // Cold LED lights below this (°C)

// ─── Fan PWM Thresholds (°C) ───────────────────────────────
#define FAN_START_TEMP   22.0f   // Fan starts at this temperature
#define FAN_FULL_TEMP    45.0f   // Fan runs at 100% above this

// ─── Timing ────────────────────────────────────────────────
#define DATA_SEND_INTERVAL   2000   // ms between server pushes
#define SENSOR_READ_INTERVAL  500   // ms between sensor reads

DHT dht(DHT_PIN, DHT_TYPE);

// ─── State Variables ───────────────────────────────────────
float temperature     = 0.0;
float humidity        = 0.0;
int   ldrValue        = 0;
int   fanSpeedPWM     = 0;    // 0–255
int   fanSpeedPercent = 0;    // 0–100
bool  alarmActive     = false;
bool  coldLEDActive   = false;
bool  isOverride      = false;
int   overrideFanPWM  = 0;

unsigned long lastSendTime   = 0;
unsigned long lastReadTime   = 0;

// ─── Forward Declarations ──────────────────────────────────
void connectWiFi();
void readSensors();
void computeFanSpeed();
void applyOutputs();
void sendDataToServer();
void fetchOverrideFromServer();
int  temperatureToPWM(float temp);

// ===========================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Smart Climate Control System Booting ===");

  // Pin modes
  pinMode(FAN_PIN,      OUTPUT);
  pinMode(BUZZER_PIN,   OUTPUT);
  pinMode(COLD_LED_PIN, OUTPUT);

  // Safe initial state
  analogWrite(FAN_PIN, 0);
  digitalWrite(BUZZER_PIN,   LOW);
  digitalWrite(COLD_LED_PIN, LOW);

  dht.begin();
  connectWiFi();

  Serial.println("=== Setup complete. Entering main loop. ===\n");
}

// ===========================================================
void loop() {
  unsigned long now = millis();

  // ── Read sensors every SENSOR_READ_INTERVAL ──
  if (now - lastReadTime >= SENSOR_READ_INTERVAL) {
    readSensors();
    computeFanSpeed();
    applyOutputs();
    lastReadTime = now;
  }

  // ── Communicate with server every DATA_SEND_INTERVAL ──
  if (now - lastSendTime >= DATA_SEND_INTERVAL) {
    if (WiFi.status() == WL_CONNECTED) {
      sendDataToServer();
      fetchOverrideFromServer();
    } else {
      Serial.println("[WiFi] Disconnected — attempting reconnect...");
      connectWiFi();
    }
    lastSendTime = now;
  }
}

// ===========================================================
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WiFi] Connecting to ");
  Serial.print(WIFI_SSID);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Connected!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[WiFi] Connection failed — running in offline mode.");
  }
}

// ===========================================================
void readSensors() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t) && !isnan(h)) {
    temperature = t;
    humidity    = h;
  } else {
    Serial.println("[DHT] Sensor read failed — using last known values.");
  }

  ldrValue = analogRead(LDR_PIN);  // 0 (dark) – 1023 (bright)
}

// ===========================================================
void computeFanSpeed() {
  if (!isOverride) {
    fanSpeedPWM = temperatureToPWM(temperature);
  } else {
    fanSpeedPWM = overrideFanPWM;
  }

  fanSpeedPercent = map(fanSpeedPWM, 0, 255, 0, 100);

  alarmActive   = (temperature > TEMP_ALARM_HIGH);
  coldLEDActive = (temperature < TEMP_COLD_LOW);
}

// ===========================================================
int temperatureToPWM(float temp) {
  if (temp <= FAN_START_TEMP) return 0;
  if (temp >= FAN_FULL_TEMP)  return 255;

  // Linear interpolation between start and full speed
  float ratio = (temp - FAN_START_TEMP) / (FAN_FULL_TEMP - FAN_START_TEMP);
  return (int)(ratio * 255);
}

// ===========================================================
void applyOutputs() {
  analogWrite(FAN_PIN, fanSpeedPWM);
  digitalWrite(BUZZER_PIN,   alarmActive   ? HIGH : LOW);
  digitalWrite(COLD_LED_PIN, coldLEDActive ? HIGH : LOW);

  // Serial monitor debug
  Serial.printf("[Sensor] Temp: %.1f°C | Hum: %.1f%% | LDR: %d | Fan: %d%% | Alarm: %s | ColdLED: %s | Override: %s\n",
    temperature, humidity, ldrValue, fanSpeedPercent,
    alarmActive   ? "ON" : "OFF",
    coldLEDActive ? "ON" : "OFF",
    isOverride    ? "ON" : "OFF");
}

// ===========================================================
void sendDataToServer() {
  WiFiClient wifiClient;
  HTTPClient http;

  String url = String(SERVER_URL) + "/api/data";
  http.begin(wifiClient, url);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> doc;
  doc["temperature"]    = temperature;
  doc["humidity"]       = humidity;
  doc["ldrValue"]       = ldrValue;
  doc["fanSpeed"]       = fanSpeedPercent;
  doc["alarmActive"]    = alarmActive;
  doc["coldLEDActive"]  = coldLEDActive;
  doc["isOverride"]     = isOverride;

  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  if (code != 200) {
    Serial.printf("[HTTP] POST /api/data failed, code: %d\n", code);
  }
  http.end();
}

// ===========================================================
void fetchOverrideFromServer() {
  WiFiClient wifiClient;
  HTTPClient http;

  String url = String(SERVER_URL) + "/api/override";
  http.begin(wifiClient, url);

  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();

    StaticJsonDocument<128> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err) {
      isOverride       = doc["isOverride"].as<bool>();
      int pct          = doc["fanSpeed"].as<int>();
      overrideFanPWM   = map(pct, 0, 100, 0, 255);
    }
  } else {
    Serial.printf("[HTTP] GET /api/override failed, code: %d\n", code);
  }
  http.end();
}
