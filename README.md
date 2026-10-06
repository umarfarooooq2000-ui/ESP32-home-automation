# 🏠 ESP32 Home Automation

A self-contained home automation controller built on a plain **ESP32 dev board**: control 4 relay
channels (lights, fan, pumps…) from your phone, and monitor **temperature, humidity and motion** —
all served from the device itself. No cloud, no subscriptions, no external server.

> 💡 **No hardware yet?** Try it in your browser first:
> ```bash
> node simulator/server.js     # → http://localhost:8080
> ```
> The simulator serves the *exact same dashboard* the ESP32 serves, with mock sensor data.

---

## Features

| | |
|---|---|
| 📱 **Built-in web dashboard** | Responsive dark UI served from the ESP32's flash — works great on phones |
| 🔌 **4 relay channels** | Custom names, active-low boards supported, states survive power cuts (NVS) |
| 🌡️ **DHT22 sensor** | Live temperature & humidity on the dashboard |
| 🏃 **PIR motion sensor** | Motion flag with cooldown window |
| 🛜 **Easy WiFi setup** | On first boot the device opens a setup access point (`ESP32-Home-Setup`) — no hardcoded passwords |
| 🔗 **REST API** | Simple JSON API for scripts, Home Assistant, Node-RED, curl… |
| 📡 **mDNS + OTA** | Reach it at `http://esp32-home.local`; update firmware over the air |

## Hardware you need

| Part | Notes |
|---|---|
| ESP32 dev board | any classic `ESP32-DevKitC` / `esp32dev` board |
| 4-channel 5 V relay module | opto-isolated, most are **active-low** |
| DHT22 (AM2302) temp/humidity sensor | DHT11 works too (change `DHT_TYPE`) |
| HC-SR501 PIR motion sensor | optional |
| 5 V PSU, jumper wires, breadboard/PCB | relay modules want 5 V for the coils |

## Wiring

| Component | Pin | ESP32 GPIO | Notes |
|---|---|---|---|
| Relay board | IN1 | **26** | |
| Relay board | IN2 | **27** | |
| Relay board | IN3 | **32** | |
| Relay board | IN4 | **33** | |
| Relay board | VCC / GND | VIN(5V) / GND | relay coils need 5 V |
| DHT22 | DATA | **4** | 10 kΩ pull-up resistor DATA→3.3 V |
| DHT22 | VCC / GND | 3V3 / GND | |
| HC-SR501 | OUT | **18** | optional |
| HC-SR501 | VCC / GND | VIN(5V) / GND | optional |

All pins and names are configurable in [`src/config.h`](src/config.h).

> ### ⚠️ Mains voltage is dangerous
> If you switch mains (110/230 V) through the relays: use proper insulated enclosures, keep
> low-voltage and mains sides physically separated, never work on it while plugged in — or better,
> stick to low-voltage (12 V LEDs, USB devices) while learning. You are responsible for your own
> safety and local electrical codes.

## Quick start

1. **Install tools** — [VS Code](https://code.visualstudio.com/) + the [PlatformIO IDE](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) extension.
2. **Open this folder** in VS Code; PlatformIO installs the toolchain + libraries automatically.
3. **Edit [`src/config.h`](src/config.h)** — relay names, pins, sensors.
4. **Upload** (USB cable → *Upload* button, or `pio run -t upload`).
5. **Configure WiFi** — on first boot the device creates an access point named `ESP32-Home-Setup`.
   Connect to it with your phone; the setup page opens automatically. Pick your home WiFi.
6. **Open the dashboard** — the serial monitor (`pio device monitor`) prints the device's IP, or
   just visit **http://esp32-home.local**.

## Configuration reference (`src/config.h`)

| Define | Default | Meaning |
|---|---|---|
| `DEVICE_NAME` | `"ESP32 Home"` | Dashboard title |
| `MDNS_HOSTNAME` | `"esp32-home"` | `http://esp32-home.local` |
| `NUM_RELAYS` | `4` | number of relay channels |
| `RELAY_PINS` | `{26,27,32,33}` | GPIOs driving IN1..IN4 |
| `RELAY_ACTIVE_LOW` | `true` | most relay boards: LOW = ON |
| `RELAY_NAMES` | rooms… | labels on the dashboard |
| `DHT_PIN` / `DHT_TYPE` | `4` / `DHT22` | temp/humidity sensor (`-1` to disable) |
| `PIR_PIN` | `18` | motion sensor (`-1` to disable) |
| `RELAY_PERSIST` | `true` | remember relay states across reboots |

## REST API

Base URL: `http://esp32-home.local` (or the device IP).

### `GET /api/status`
```json
{
  "device": "ESP32 Home", "ip": "192.168.1.84", "rssi": -58,
  "uptime": 3602, "freeHeap": 208332,
  "relays": [ {"name": "Living Room Light", "on": true}, {"name": "Bedroom Light", "on": false},
              {"name": "Kitchen Light", "on": false}, {"name": "Ceiling Fan", "on": false} ],
  "sensors": { "temperature": 23.4, "humidity": 51.2, "motion": false }
}
```

### `POST /api/relay` — set or toggle one channel
```bash
curl -X POST http://esp32-home.local/api/relay \
     -H "Content-Type: application/json" \
     -d '{"channel":0,"state":true}'      # or {"channel":0,"toggle":true}
```

### `POST /api/relays` — set all channels at once
```bash
curl -X POST http://esp32-home.local/api/relays \
     -H "Content-Type: application/json" \
     -d '{"state":false}'
```

Both POST endpoints return the full status document, so one call = command + confirmation.

### Home Assistant example
Use a [REST switch](https://www.home-assistant.io/integrations/switch.rest/):
```yaml
switch:
  - platform: rest
    resource: http://esp32-home.local/api/relay
    name: Living room light
    body_on:  '{"channel":0,"state":true}'
    body_off: '{"channel":0,"state":false}'
    is_on_template: "{{ value_json.relays[0].on }}"
```

## Simulator

`simulator/server.js` serves the real dashboard (extracted live from `src/dashboard.h`) with a mock
API and drifting sensor values — handy for UI tweaking without flashing:

```bash
node simulator/server.js          # http://localhost:8080  (PORT=xxx to change)
```

## Project structure

```
├── platformio.ini        # PlatformIO build config (board + libraries)
├── src/
│   ├── main.cpp          # firmware: WiFi, relays, sensors, REST API, OTA
│   ├── config.h          # ✏️ all user-tunable settings (pins, names…)
│   └── dashboard.h       # web UI served from the ESP32 (raw string literal)
├── simulator/
│   └── server.js         # browser simulator — same UI, mock sensors
└── README.md
```

## Roadmap ideas

- [ ] MQTT + Home Assistant auto-discovery
- [ ] Schedules / timers ("fan on for 30 min")
- [ ] Physical wall buttons (GPIO interrupts)
- [ ] BME280 / DS18B20 sensors, multiple devices
- [ ] Google Home / Alexa via Home Assistant bridge

## License

[MIT](LICENSE) — free to use, modify and share.
