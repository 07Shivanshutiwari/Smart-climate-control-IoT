# 🌡️ IoT-Based Smart Ambient Climate Control System

> A full-stack IoT project combining embedded hardware with a real-time web dashboard for remote monitoring and control of ambient temperature and fan speed.

---

## 📌 Overview

This project was built as a college embedded systems project and later extended into a complete IoT solution. The system automatically controls fan speed based on ambient temperature using a NodeMCU (ESP8266) microcontroller, and exposes a live web dashboard for remote monitoring and manual override.

**Original hardware project features:**
- Auto fan speed control based on temperature (PWM)
- High-temperature buzzer alarm (> 60°C)
- Cold indicator LED (< 18°C)
- Ambient light sensing via LDR

**Added for IoT/full-stack version:**
- WiFi connectivity via ESP8266
- Node.js backend server
- Real-time web dashboard with live charts
- Remote fan speed override from any browser

---

## 🖥️ Dashboard Preview

The web dashboard shows:
- Live temperature, humidity, and light readings
- Animated fan with real-time speed indicator
- High-temp alarm and cold LED status
- Fan speed override slider and presets
- 2-minute live history chart

---

## 🏗️ System Architecture

```
┌─────────────────────┐        HTTP POST        ┌──────────────────────┐
│   NodeMCU ESP8266   │ ─────── /api/data ──────▶│                      │
│                     │                          │   Node.js Server     │
│  • DHT11 Sensor     │ ◀────── /api/override ───│   (Express.js)       │
│  • PWM Fan Control  │        HTTP GET          │                      │
│  • LDR              │                          └──────────┬───────────┘
│  • Buzzer           │                                     │
│  • Cold LED         │                          HTTP GET   │ /api/status
└─────────────────────┘                                     ▼
                                                  ┌──────────────────────┐
                                                  │   Web Dashboard      │
                                                  │   (HTML/CSS/JS)      │
                                                  │   Chart.js graphs    │
                                                  └──────────────────────┘
```

---

## 🛠️ Tech Stack

| Layer        | Technology                          |
|--------------|-------------------------------------|
| Hardware     | NodeMCU ESP8266, DHT11, PWM Fan     |
| Firmware     | Arduino C++ (ESP8266 SDK)           |
| Backend      | Node.js, Express.js                 |
| Frontend     | HTML5, CSS3, Vanilla JavaScript     |
| Charts       | Chart.js                            |
| Protocol     | HTTP REST (Arduino ↔ Server)        |

---

## 📁 Project Structure

```
smart-climate-control-iot/
├── arduino/
│   └── smart_climate_control.ino   # NodeMCU firmware
├── server/
│   ├── server.js                   # Express.js backend
│   └── package.json
├── public/
│   └── index.html                  # Web dashboard (single file)
├── docs/
│   └── wiring_guide.md             # Circuit & pin connections
├── .gitignore
└── README.md
```

---

## ⚡ Getting Started

### 1. Hardware Setup
Follow [`docs/wiring_guide.md`](docs/wiring_guide.md) for full circuit diagram and pin connections.

**Required components:**
- NodeMCU ESP8266 (or Arduino Uno + ESP8266 module)
- DHT11 temperature & humidity sensor
- 5V/12V DC fan + NPN transistor (2N2222 or TIP120)
- LDR (photoresistor) + 10kΩ resistor
- Active buzzer
- LED + 220Ω resistor

### 2. Flash Arduino Firmware

**Install required libraries** (Arduino IDE → Manage Libraries):
- `DHT sensor library` by Adafruit
- `ArduinoJson` by Benoit Blanchon

**Add ESP8266 board support:**
```
http://arduino.esp8266.com/stable/package_esp8266com_index.json
```

**Edit credentials** in `arduino/smart_climate_control.ino`:
```cpp
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* SERVER_URL    = "http://YOUR_SERVER_IP:3000";
```

Upload to NodeMCU with board set to **NodeMCU 1.0 (ESP-12E Module)**.

### 3. Run the Server

```bash
cd server
npm install
npm start
```

Server starts at `http://localhost:3000`

### 4. Open the Dashboard

Open your browser and go to:
```
http://localhost:3000
```

Once the NodeMCU is powered and on the same WiFi network, you'll see live data within 2–3 seconds.

---

## 🌡️ Fan Speed Logic

| Temperature      | Fan Speed           |
|------------------|---------------------|
| Below 22°C       | OFF (0%)            |
| 22°C – 45°C      | Linear 0% → 100%    |
| Above 45°C       | Maximum (100%)      |

Fan speed can be **manually overridden** from the web dashboard at any time.

---

## 🔔 Alerts

| Condition         | Response            |
|-------------------|---------------------|
| Temperature > 60°C | Buzzer alarm + dashboard alert |
| Temperature < 18°C | Cold indicator LED ON |

---

## 📡 API Reference

| Method | Endpoint        | Description                        |
|--------|-----------------|------------------------------------|
| POST   | `/api/data`     | Arduino pushes sensor readings     |
| GET    | `/api/status`   | Dashboard reads latest data        |
| GET    | `/api/override` | Arduino polls for override command |
| POST   | `/api/override` | Dashboard sets fan override        |
| GET    | `/api/history`  | Fetch last 60 data points          |
| GET    | `/api/health`   | Server health check                |

---


## 📄 License

MIT License — feel free to use, modify, and build on this project.
