/*
 * =====================================================================
 *  ESP32 Home Automation
 *  -------------------------------------------------------------------
 *  - 4 relay channels (lights / fan / anything), state persisted in NVS
 *  - DHT22 temperature + humidity, PIR motion sensor
 *  - Built-in web dashboard + JSON REST API (see README.md)
 *  - WiFi provisioning via fallback access point (WiFiManager)
 *  - mDNS: http://esp32-home.local   + OTA firmware updates
 *
 *  Wiring, configuration and build instructions: see README.md
 *  All user-tunable options live in config.h
 * =====================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFiManager.h>      // https://github.com/tzapu/WiFiManager
#include <DHT.h>              // Adafruit DHT sensor library
#include <ArduinoOTA.h>

#include "config.h"
#include "dashboard.h"

// ---------------------------------------------------------------- state
static int           relayPins[NUM_RELAYS]      = RELAY_PINS;
static const char*   relayNames[NUM_RELAYS]     = RELAY_NAMES;
static bool          relayState[NUM_RELAYS];

static DHT           dht(DHT_PIN, DHT_TYPE);
static float         temperature = NAN;
static float         humidity    = NAN;
static unsigned long lastDhtRead = 0;

static volatile unsigned long motionLastTrigger = 0;   // updated from ISR
static bool motionActive() {
  return (millis() - motionLastTrigger) < PIR_COOLDOWN_MS;
}
void IRAM_ATTR onMotionISR() { motionLastTrigger = millis(); }

static WebServer     server(80);
static Preferences   prefs;
static unsigned long lastStatusLog = 0;

// ------------------------------------------------------------- relays
// Maps a logical ON/OFF to the GPIO level, honouring RELAY_ACTIVE_LOW.
static void writeRelay(uint8_t ch) {
  bool on = relayState[ch];
  digitalWrite(relayPins[ch], (RELAY_ACTIVE_LOW ? !on : on) ? HIGH : LOW);
}

static void saveRelayStates() {
  if (!RELAY_PERSIST) return;
  uint8_t mask = 0;
  for (uint8_t i = 0; i < NUM_RELAYS; i++)
    if (relayState[i]) mask |= (1 << i);
  prefs.putUChar("relayMask", mask);
}

static void loadRelayStates() {
  uint8_t mask = RELAY_PERSIST ? prefs.getUChar("relayMask", 0) : 0;
  for (uint8_t i = 0; i < NUM_RELAYS; i++)
    relayState[i] = (mask >> i) & 1;
}

// ------------------------------------------------------------ REST API
static void sendStatus() {
  JsonDocument doc;
  doc["device"]   = DEVICE_NAME;
  doc["ip"]       = WiFi.localIP().toString();
  doc["rssi"]     = WiFi.RSSI();
  doc["uptime"]   = (uint32_t)(millis() / 1000);
  doc["freeHeap"] = ESP.getFreeHeap();

  JsonArray arr = doc["relays"].to<JsonArray>();
  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    JsonObject r = arr.add<JsonObject>();
    r["name"] = relayNames[i];
    r["on"]   = relayState[i];
  }

  JsonObject s = doc["sensors"].to<JsonObject>();
  s["temperature"] = temperature;   // NaN serialises as JSON null
  s["humidity"]    = humidity;
  s["motion"]      = motionActive();

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

// POST /api/relay   {"channel":0,"state":true}   or   {"channel":0,"toggle":true}
static void handleRelay() {
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"error\":\"invalid JSON body\"}");
    return;
  }
  int ch = doc["channel"] | -1;
  if (ch < 0 || ch >= NUM_RELAYS) {
    server.send(400, "application/json", "{\"error\":\"channel out of range\"}");
    return;
  }
  if (doc["toggle"].is<bool>()) {
    relayState[ch] = !relayState[ch];
  } else if (doc["state"].is<bool>()) {
    relayState[ch] = doc["state"].as<bool>();
  } else {
    server.send(400, "application/json", "{\"error\":\"provide 'state' or 'toggle'\"}");
    return;
  }
  writeRelay(ch);
  saveRelayStates();
  Serial.printf("[api] relay %d (%s) -> %s\n", ch, relayNames[ch], relayState[ch] ? "ON" : "OFF");
  sendStatus();
}

// POST /api/relays  {"state":false}   -> set every channel at once
static void handleAllRelays() {
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain")) || !doc["state"].is<bool>()) {
    server.send(400, "application/json", "{\"error\":\"expected {\\\"state\\\":true|false}\"}");
    return;
  }
  bool on = doc["state"].as<bool>();
  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    relayState[i] = on;
    writeRelay(i);
  }
  saveRelayStates();
  Serial.printf("[api] all relays -> %s\n", on ? "ON" : "OFF");
  sendStatus();
}

static void handleNotFound() {
  String body = "{\"error\":\"not found\",\"path\":\"" + server.uri() + "\"}";
  server.send(404, "application/json", body);
}

// ------------------------------------------------------------- logging
static void printStatus() {
  Serial.printf("[status] up=%lus heap=%uB wifi=%ddBm  temp=%.1fC hum=%.1f%% motion=%d  relays:",
                (unsigned long)(millis() / 1000), ESP.getFreeHeap(), WiFi.RSSI(),
                temperature, humidity, motionActive());
  for (uint8_t i = 0; i < NUM_RELAYS; i++) Serial.printf(" %d", relayState[i]);
  Serial.println();
}

// --------------------------------------------------------------- setup
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== ESP32 Home Automation ===");

  // Force relays to a known level *immediately* — floating inputs on
  // active-low relay boards can flicker channels ON during boot.
  prefs.begin("homeauto", false);
  loadRelayStates();
  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    pinMode(relayPins[i], OUTPUT);
    writeRelay(i);
  }

#if DHT_PIN >= 0
  dht.begin();
#endif
#if PIR_PIN >= 0
  pinMode(PIR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), onMotionISR, RISING);
#endif

  // ---- WiFi: try saved credentials, else open the setup portal ----
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setConfigPortalTimeout(AP_PORTAL_TIMEOUT_S);
  if (!wm.autoConnect(AP_SSID)) {
    Serial.println("[wifi] setup portal timed out, restarting...");
    delay(1000);
    ESP.restart();
  }
  Serial.printf("[wifi] connected: %s (RSSI %d dBm)\n",
                WiFi.localIP().toString().c_str(), WiFi.RSSI());
  Serial.printf("[wifi] dashboard: http://%s.local\n", MDNS_HOSTNAME);

  if (MDNS.begin(MDNS_HOSTNAME)) MDNS.addService("http", "tcp", 80);

  ArduinoOTA.setHostname(MDNS_HOSTNAME);
  // ArduinoOTA.setPassword("change-me");   // enable for a bit more safety
  ArduinoOTA.begin();

  // ---- HTTP server ----
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", DASHBOARD_HTML);
  });
  server.on("/api/status", HTTP_GET, sendStatus);
  server.on("/api/relay",  HTTP_POST, handleRelay);
  server.on("/api/relays", HTTP_POST, handleAllRelays);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[http] server started on port 80");
}

// ---------------------------------------------------------------- loop
void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  unsigned long now = millis();

#if DHT_PIN >= 0
  if (now - lastDhtRead >= DHT_READ_MS) {
    lastDhtRead = now;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) temperature = t;   // keep last good value on read errors
    if (!isnan(h)) humidity = h;
  }
#endif

  if (now - lastStatusLog >= STATUS_LOG_MS) {
    lastStatusLog = now;
    printStatus();
  }
}
