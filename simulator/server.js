#!/usr/bin/env node
/*
 * ESP32 Home Automation — browser simulator
 * ------------------------------------------------
 * Serves the *exact same* dashboard that the ESP32 firmware serves
 * (extracted from src/dashboard.h) with a mock /api backend and fake
 * sensor data, so you can try the UI before any hardware is wired up.
 *
 * Usage:  node simulator/server.js     (then open http://localhost:8080)
 * Needs Node.js >= 16, no npm install required.
 */

'use strict';

const http = require('http');
const fs = require('fs');
const path = require('path');

const PORT = process.env.PORT || 8080;
const HOST = '0.0.0.0';

// ---- pull the dashboard HTML out of the firmware source -------------------
const dashboardHtml = (() => {
  const src = fs.readFileSync(path.join(__dirname, '..', 'src', 'dashboard.h'), 'utf8');
  const m = src.match(/DASHBOARD_HTML\[\]\s*PROGMEM\s*=\s*R"rawliteral\(([\s\S]*?)\)rawliteral"/);
  if (!m) throw new Error('Could not extract dashboard HTML from src/dashboard.h');
  return m[1];
})();

// ---- mock device state (mirrors firmware config.h defaults) ---------------
const relays = ['Living Room Light', 'Bedroom Light', 'Kitchen Light', 'Ceiling Fan']
  .map((name) => ({ name, on: false }));

let temperature = 22.4;
let humidity = 47.1;
let motionUntil = 0;
const startedAt = Date.now();

function status() {
  // gentle random-walk so the tiles feel alive
  temperature = Math.min(30, Math.max(18, temperature + (Math.random() - 0.5) * 0.15));
  humidity = Math.min(70, Math.max(30, humidity + (Math.random() - 0.5) * 0.6));

  // occasionally trip the (simulated) motion sensor
  if (Date.now() > motionUntil && Math.random() < 0.03) {
    motionUntil = Date.now() + 5000;
  }

  return {
    device: 'ESP32 Home',
    ip: 'simulated',
    rssi: Math.round(-50 - Math.random() * 15),
    uptime: Math.floor((Date.now() - startedAt) / 1000),
    freeHeap: 195000 + Math.floor(Math.random() * 20000),
    relays: relays.map((r) => ({ name: r.name, on: r.on })),
    sensors: {
      temperature: Math.round(temperature * 10) / 10,
      humidity: Math.round(humidity * 10) / 10,
      motion: Date.now() < motionUntil,
    },
  };
}

// ---- tiny helpers ---------------------------------------------------------
function json(res, code, obj) {
  const body = JSON.stringify(obj);
  res.writeHead(code, {
    'Content-Type': 'application/json',
    'Content-Length': Buffer.byteLength(body),
  });
  res.end(body);
}

function readBody(req) {
  return new Promise((resolve) => {
    let data = '';
    req.on('data', (c) => (data += c));
    req.on('end', () => resolve(data));
  });
}

// ---- server ----------------------------------------------------------------
const server = http.createServer(async (req, res) => {
  const url = new URL(req.url, `http://${req.headers.host || 'localhost'}`);

  if (req.method === 'GET' && (url.pathname === '/' || url.pathname === '/index.html')) {
    res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
    return res.end(dashboardHtml);
  }

  if (req.method === 'GET' && url.pathname === '/api/status') {
    return json(res, 200, status());
  }

  if (req.method === 'POST' && url.pathname === '/api/relay') {
    try {
      const payload = JSON.parse((await readBody(req)) || '{}');
      const ch = payload.channel;
      if (!Number.isInteger(ch) || ch < 0 || ch >= relays.length) {
        return json(res, 400, { error: 'channel out of range' });
      }
      relays[ch].on = payload.toggle ? !relays[ch].on : !!payload.state;
      console.log(`[sim] ${relays[ch].name} -> ${relays[ch].on ? 'ON' : 'off'}`);
      return json(res, 200, status());
    } catch {
      return json(res, 400, { error: 'invalid JSON body' });
    }
  }

  if (req.method === 'POST' && url.pathname === '/api/relays') {
    try {
      const payload = JSON.parse((await readBody(req)) || '{}');
      if (typeof payload.state !== 'boolean') {
        return json(res, 400, { error: 'expected {"state":true|false}' });
      }
      relays.forEach((r) => (r.on = payload.state));
      console.log(`[sim] all relays -> ${payload.state ? 'ON' : 'off'}`);
      return json(res, 200, status());
    } catch {
      return json(res, 400, { error: 'invalid JSON body' });
    }
  }

  return json(res, 404, { error: 'not found', path: url.pathname });
});

server.listen(PORT, HOST, () => {
  console.log(`ESP32 Home simulator running → http://localhost:${PORT}`);
  console.log('Serving the same dashboard the firmware serves, with mock sensor data.\n');
});
