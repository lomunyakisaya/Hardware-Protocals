#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

// WiFi settings
const char* ssid = "lomunyak";
const char* password = "Ilm..1703"; 

// NodeMCU SoftwareSerial: RX=5, TX=4
const uint8_t NODEMCU_RX_PIN = 5;
const uint8_t NODEMCU_TX_PIN = 4;
const uint32_t UART_BAUD = 9600;
SoftwareSerial megaSerial(NODEMCU_RX_PIN, NODEMCU_TX_PIN);

ESP8266WebServer server(80);

String serialBuffer = "";
String sensorJson = "{\"joystickX\":0,\"joystickY\":0,\"speed\":0,\"distance\":0,\"servo\":0,\"status\":\"idle\"}";

String buildWebPage() {
  return R"HTML(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Drone Sensor Dashboard</title>
  <style>
    :root {
      color-scheme: dark;
    }
    body {
      font-family: Arial, sans-serif;
      background: linear-gradient(180deg, #08111f, #0f172a);
      color: #f8fafc;
      margin: 0;
      padding: 24px;
    }
    h1 {
      margin: 0 0 12px;
      font-size: 1.8rem;
    }
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
      gap: 12px;
      margin-bottom: 16px;
    }
    .card {
      background: rgba(15, 23, 42, 0.92);
      border: 1px solid #334155;
      border-radius: 14px;
      padding: 14px;
      box-shadow: 0 8px 24px rgba(0,0,0,0.25);
    }
    .label {
      color: #7dd3fc;
      font-weight: bold;
      display: block;
      margin-bottom: 4px;
    }
    .value {
      font-size: 1.15rem;
      font-weight: 700;
    }
    .status {
      display: inline-block;
      margin-top: 12px;
      padding: 8px 12px;
      border-radius: 999px;
      background: #164e63;
      color: #cffafe;
      font-weight: 700;
    }
    .status.warn { background: #78350f; color: #fde68a; }
    .status.ok { background: #14532d; color: #bbf7d0; }
    canvas {
      width: 100%;
      max-width: 520px;
      background: #0b1220;
      border-radius: 14px;
      border: 1px solid #263247;
      box-shadow: 0 8px 24px rgba(0,0,0,0.25);
    }
  </style>
</head>
<body>
  <h1>Drone Sensor Simulation</h1>
  <div class="grid">
    <div class="card"><span class="label">Joystick X</span><div class="value" id="joyX">0</div></div>
    <div class="card"><span class="label">Joystick Y</span><div class="value" id="joyY">0</div></div>
    <div class="card"><span class="label">Speed</span><div class="value" id="speed">0</div></div>
    <div class="card"><span class="label">Distance</span><div class="value" id="distance">0</div></div>
    <div class="card"><span class="label">Servo</span><div class="value" id="servo">0</div></div>
    <div class="card"><span class="label">Status</span><div class="value" id="status">idle</div></div>
  </div>

  <div id="connectionState" class="status">Waiting for drone telemetry…</div>
  <div style="margin-top: 16px;">
    <canvas id="droneCanvas" width="520" height="280"></canvas>
  </div>

  <script>
    const canvas = document.getElementById('droneCanvas');
    const ctx = canvas.getContext('2d');
    const connectionState = document.getElementById('connectionState');

    const liveData = {
      joystickX: 0,
      joystickY: 0,
      speed: 0,
      distance: 0,
      servo: 0,
      status: 'idle'
    };

    const visualData = {
      joystickX: 0,
      joystickY: 0,
      servo: 0,
      distance: 0
    };

    let lastUpdate = 0;

    function setConnectionState(text, mode) {
      connectionState.textContent = text;
      connectionState.className = 'status';
      if (mode === 'ok') connectionState.classList.add('ok');
      if (mode === 'warn') connectionState.classList.add('warn');
    }

    function smoothValue(target, current, factor) {
      return current + (target - current) * factor;
    }

    function drawDrone() {
      const centerX = 260 + (visualData.joystickX - 512) / 14;
      const centerY = 140 + (visualData.joystickY - 512) / 14;
      const tilt = (visualData.servo - 90) * 0.75;
      const altitude = Math.max(40, 152 - visualData.distance * 2);

      ctx.clearRect(0, 0, canvas.width, canvas.height);
      ctx.fillStyle = '#08111f';
      ctx.fillRect(0, 0, canvas.width, canvas.height);

      ctx.strokeStyle = 'rgba(74, 222, 128, 0.35)';
      ctx.lineWidth = 1;
      for (let y = 20; y < canvas.height; y += 32) {
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(canvas.width, y);
        ctx.stroke();
      }
      for (let x = 20; x < canvas.width; x += 32) {
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, canvas.height);
        ctx.stroke();
      }

      ctx.strokeStyle = '#4ade80';
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.moveTo(20, 150);
      ctx.lineTo(500, 150);
      ctx.stroke();

      ctx.save();
      ctx.translate(centerX, centerY);
      ctx.rotate(tilt * Math.PI / 180);

      ctx.strokeStyle = '#38bdf8';
      ctx.lineWidth = 4;
      ctx.beginPath();
      ctx.moveTo(-30, -10);
      ctx.lineTo(-30, 10);
      ctx.moveTo(30, -10);
      ctx.lineTo(30, 10);
      ctx.moveTo(-10, -30);
      ctx.lineTo(10, -30);
      ctx.moveTo(-10, 30);
      ctx.lineTo(10, 30);
      ctx.stroke();

      ctx.fillStyle = '#f97316';
      ctx.fillRect(-28, -18, 56, 36);
      ctx.fillStyle = '#facc15';
      ctx.fillRect(-12, -10, 24, 20);
      ctx.strokeStyle = '#e2e8f0';
      ctx.strokeRect(-28, -18, 56, 36);
      ctx.restore();

      ctx.fillStyle = '#94a3b8';
      ctx.fillRect(100, altitude, 220, 8);
      ctx.fillStyle = '#60a5fa';
      ctx.fillRect(108, altitude - 18, 8, 18);
      ctx.fillRect(114, altitude - 12, 8, 12);
    }

    function animate() {
      visualData.joystickX = smoothValue(liveData.joystickX, visualData.joystickX, 0.12);
      visualData.joystickY = smoothValue(liveData.joystickY, visualData.joystickY, 0.12);
      visualData.servo = smoothValue(liveData.servo, visualData.servo, 0.12);
      visualData.distance = smoothValue(liveData.distance, visualData.distance, 0.12);
      drawDrone();
      requestAnimationFrame(animate);
    }

    async function refreshData() {
      try {
        const res = await fetch('/api/sensors', { cache: 'no-store' });
        if (!res.ok) throw new Error('HTTP ' + res.status);
        const data = await res.json();

        document.getElementById('joyX').textContent = data.joystickX;
        document.getElementById('joyY').textContent = data.joystickY;
        document.getElementById('speed').textContent = data.speed;
        document.getElementById('distance').textContent = data.distance;
        document.getElementById('servo').textContent = data.servo;
        document.getElementById('status').textContent = data.status;

        liveData.joystickX = Number(data.joystickX || 0);
        liveData.joystickY = Number(data.joystickY || 0);
        liveData.speed = Number(data.speed || 0);
        liveData.distance = Number(data.distance || 0);
        liveData.servo = Number(data.servo || 0);
        liveData.status = data.status || 'idle';
        lastUpdate = Date.now();
        setConnectionState('Telemetry live — drone is updating', 'ok');
      } catch (error) {
        const stale = Date.now() - lastUpdate;
        if (stale > 2000) {
          setConnectionState('Waiting for data from Mega…', 'warn');
        }
      }
    }

    setInterval(refreshData, 250);
    requestAnimationFrame(animate);
    refreshData();
  </script>
</body>
</html>
)HTML";
}

void handleRoot() {
  server.send(200, "text/html", buildWebPage());
}

void handleSensors() {
  server.send(200, "application/json", sensorJson);
}

bool parseIncomingJson(const String& payload) {
  StaticJsonDocument<128> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    return false;
  }

  if (doc.containsKey("joystickX") && doc.containsKey("joystickY") && doc.containsKey("speed")) {
    sensorJson = payload;
    return true;
  }

  return false;
}

void readSerialData() {
  while (megaSerial.available() > 0) {
    char c = (char)megaSerial.read();

    if (c == '\r' || c == '\n') {
      if (serialBuffer.length() > 0) {
        parseIncomingJson(serialBuffer);
        serialBuffer = "";
      }
    } else if (serialBuffer.length() < 256) {
      serialBuffer += c;
    }
  }
}

void setup() {
  megaSerial.begin(UART_BAUD);
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.hostname("drone-dashboard");
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);

  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/api/sensors", handleSensors);
  server.begin();
}

void loop() {
  server.handleClient();
  readSerialData();
}
