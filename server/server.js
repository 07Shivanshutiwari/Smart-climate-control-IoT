/**
 * Smart Ambient Climate Control — Backend Server
 * ================================================
 * Receives real-time sensor data from the NodeMCU (ESP8266)
 * and serves it to the web dashboard.
 * Also holds fan-override settings that the Arduino polls.
 *
 * Routes:
 *   POST /api/data      ← Arduino pushes sensor readings here
 *   GET  /api/status    ← Dashboard reads live sensor data
 *   GET  /api/override  ← Arduino polls for manual override
 *   POST /api/override  ← Dashboard sets manual override
 *   GET  /api/history   ← Dashboard fetches recent history
 */

const express = require("express");
const cors    = require("cors");
const path    = require("path");
const http    = require("http");

const app    = express();
const server = http.createServer(app);
const PORT   = process.env.PORT || 3000;

// ─── Middleware ────────────────────────────────────────────
app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, "public")));

// ─── In-Memory State ──────────────────────────────────────
let sensorData = {
  temperature:   0,
  humidity:      0,
  ldrValue:      0,
  fanSpeed:      0,
  alarmActive:   false,
  coldLEDActive: false,
  isOverride:    false,
  lastUpdated:   null,
  deviceOnline:  false,
};

let overrideSettings = {
  isOverride: false,
  fanSpeed:   0,
};

// Rolling history: last 60 data points (2 min at 2 s interval)
const HISTORY_MAX = 60;
const history = [];

// Mark device offline if no data in 10 seconds
let lastDataTimestamp = Date.now();
setInterval(() => {
  const elapsed = Date.now() - lastDataTimestamp;
  sensorData.deviceOnline = elapsed < 10000;
}, 2000);

// ─── Routes ───────────────────────────────────────────────

/**
 * Arduino → Server
 * POST /api/data
 */
app.post("/api/data", (req, res) => {
  const {
    temperature,
    humidity,
    ldrValue,
    fanSpeed,
    alarmActive,
    coldLEDActive,
    isOverride,
  } = req.body;

  sensorData = {
    temperature:   parseFloat(temperature)   || 0,
    humidity:      parseFloat(humidity)      || 0,
    ldrValue:      parseInt(ldrValue)        || 0,
    fanSpeed:      parseInt(fanSpeed)        || 0,
    alarmActive:   Boolean(alarmActive),
    coldLEDActive: Boolean(coldLEDActive),
    isOverride:    Boolean(isOverride),
    lastUpdated:   new Date().toISOString(),
    deviceOnline:  true,
  };

  lastDataTimestamp = Date.now();

  // Append to rolling history
  history.push({
    time:        sensorData.lastUpdated,
    temperature: sensorData.temperature,
    humidity:    sensorData.humidity,
    fanSpeed:    sensorData.fanSpeed,
  });
  if (history.length > HISTORY_MAX) history.shift();

  res.json({ success: true });
});

/**
 * Dashboard ← Server (current readings)
 * GET /api/status
 */
app.get("/api/status", (req, res) => {
  res.json(sensorData);
});

/**
 * Arduino ← Server (fan override command)
 * GET /api/override
 */
app.get("/api/override", (req, res) => {
  res.json(overrideSettings);
});

/**
 * Dashboard → Server (set fan override)
 * POST /api/override
 * Body: { isOverride: boolean, fanSpeed: 0-100 }
 */
app.post("/api/override", (req, res) => {
  const { isOverride, fanSpeed } = req.body;

  if (typeof isOverride !== "boolean") {
    return res.status(400).json({ error: "isOverride must be a boolean" });
  }
  if (typeof fanSpeed !== "number" || fanSpeed < 0 || fanSpeed > 100) {
    return res.status(400).json({ error: "fanSpeed must be a number 0–100" });
  }

  overrideSettings = { isOverride, fanSpeed };
  console.log(`[Override] Mode: ${isOverride ? "MANUAL" : "AUTO"} | Speed: ${fanSpeed}%`);

  res.json({ success: true, overrideSettings });
});

/**
 * Dashboard ← Server (historical readings for chart)
 * GET /api/history
 */
app.get("/api/history", (req, res) => {
  res.json(history);
});

// ─── Health Check ─────────────────────────────────────────
app.get("/api/health", (req, res) => {
  res.json({
    status:    "ok",
    uptime:    process.uptime(),
    timestamp: new Date().toISOString(),
  });
});

// ─── Catch-All → SPA ──────────────────────────────────────
app.get("*any", (req, res) => {
  res.sendFile(path.join(__dirname, "public/index.html"));
});

// ─── Start ────────────────────────────────────────────────
server.listen(PORT, () => {
  console.log(`\n✅  Smart Climate Server running at http://localhost:${PORT}`);
  console.log(`    Waiting for NodeMCU to connect...\n`);
});
