#pragma once

// ======================================================================
//  ESP32 Home Automation — user configuration
//  Edit this file to match your wiring, then upload the firmware.
// ======================================================================

// ---------------- Device identity ----------------
#define DEVICE_NAME     "ESP32 Home"         // shown on the dashboard + browser tab
#define MDNS_HOSTNAME   "esp32-home"         // reachable at http://esp32-home.local

// ---------------- Relay outputs ----------------
// A standard 4-channel relay board. Most cheap boards are ACTIVE-LOW
// (LOW signal = relay ON). Set RELAY_ACTIVE_LOW to false if yours is
// active-high.
#define NUM_RELAYS        4
#define RELAY_PINS        { 26, 27, 32, 33 }          // IN1..IN4 -> GPIOs
#define RELAY_ACTIVE_LOW  true
#define RELAY_NAMES       { "Living Room Light", \
                            "Bedroom Light",    \
                            "Kitchen Light",    \
                            "Ceiling Fan" }     // labels shown on the dashboard

// ---------------- Sensors ----------------
#define DHT_PIN         4                       // DHT22 / DHT11 data pin (-1 to disable)
#define DHT_TYPE        DHT22                   // DHT11, DHT21 or DHT22
#define DHT_READ_MS     5000                    // sensor poll interval (min 2000 for DHT22)

#define PIR_PIN         18                      // PIR motion sensor OUT pin (-1 to disable)
#define PIR_COOLDOWN_MS 10000                   // how long "motion" stays flagged after a trigger

// ---------------- WiFi ----------------
// On first boot (or if the saved network is unreachable) the device
// starts a configuration access point. Connect your phone/laptop to
// this SSID and the setup page opens automatically.
#define AP_SSID             "ESP32-Home-Setup"
#define AP_PORTAL_TIMEOUT_S 180                 // seconds before it reboots and retries

// ---------------- Behaviour ----------------
#define RELAY_PERSIST   true    // remember relay states across power cycles (NVS)
#define STATUS_LOG_MS   10000   // interval for the serial status printout
