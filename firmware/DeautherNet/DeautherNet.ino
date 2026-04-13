#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "CustomRadioHooks.h"

// ==========================================
// EMBEDDED HTML INTERFACE (LITE VERSION 4.0)
// ==========================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>BruceForce Lab</title>
    <style>
        body { background: #0d1117; color: #58a6ff; font-family: monospace; padding: 20px; }
        .card { border: 1px solid #30363d; background: #161b22; padding: 15px; margin-bottom: 15px; border-radius: 8px; }
        button { background: #238636; color: white; border: none; padding: 10px 15px; border-radius: 6px; cursor: pointer; margin-bottom: 5px; font-weight: bold; }
        button.attack { background: #da3633; }
        button.scan { background: #1f6feb; }
        button.clear { background: #484f58; }
        button.inline-btn { padding: 4px 8px; font-size: 11px; margin-left: 10px; }
        input { background: #0d1117; color: white; border: 1px solid #30363d; padding: 8px; border-radius: 6px; margin-bottom: 10px; width: 90%; }
        .row-input { display: flex; gap: 10px; align-items: center; }
        .row-input input { width: 100%; margin-bottom: 0; }
        h4 { margin: 0 0 10px 0; color: #f0f6fc; }
        #log, #scan-results { height: 120px; overflow-y: auto; background: #000; padding: 10px; font-size: 11px; color: #8b949e; border-radius: 6px; line-height: 1.8; }
        #scan-results { color: #d2a8ff; margin-bottom: 10px; }
    </style>
</head>
<body>
    <h3>BRUCEFORCE LAB</h3>
    <div class="card">
        Status: <span id="stat" style="color:#f85149">OFFLINE</span> | <button onclick="sync()">SYNC</button>
    </div>
    
    <div class="card">
        <h4>SCAN & DEAUTH</h4>
        <button onclick="runAction('SCAN_START')">1. SCAN</button>
        <button class="scan" onclick="getScans()">2. RESULTS</button>
        <button class="clear" onclick="clearLog()">CLEAR LOG</button>
        <div id="scan-results" style="margin-top: 10px;">Networks will appear here...</div>
    </div>

    <div class="card">
        <h4>BEACON SPAMMER</h4>
        <div class="row-input">
            <input type="text" id="bName" placeholder="SSID Name (e.g. Free_WiFi)">
            <input type="number" id="bCount" value="10" min="1" max="60" placeholder="Qty" style="width: 70px;">
        </div>
        <br>
        <button class="attack" onclick="startBeacon()">START BEACONS</button>
        <button class="clear" onclick="clearLog()">CLEAR LOG</button>
    </div>

    <div id="log"></div>

    <script>
        async function sync() {
            try {
                const r = await fetch('/status');
                const d = await r.json();
                document.getElementById('stat').innerText = "ONLINE";
                document.getElementById('stat').style.color = "#3fb950";
                document.getElementById('log').innerHTML += "> Synced (" + d.ap_ssid + ")<br>";
            } catch(e) { document.getElementById('log').innerHTML += "> Sync Failed<br>"; }
        }

        async function runAction(act) {
            try {
                await fetch('/api/action', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({ action: act, names: [] })
                });
                document.getElementById('log').innerHTML += `> Sent: ${act}<br>`;
            } catch(e) { document.getElementById('log').innerHTML += "> HTTP Error<br>"; }
        }

        async function startDeauth(mac, ch, ssid) {
            try {
                await fetch('/api/action', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({ action: 'ATTACK_DEAUTH', names: [`${mac},${ch}`] })
                });
                document.getElementById('log').innerHTML += `> <b>DEAUTHING:</b> ${ssid} (Ch: ${ch})<br>`;
            } catch(e) { document.getElementById('log').innerHTML += "> HTTP Error<br>"; }
        }

        async function startBeacon() {
            const bName = document.getElementById('bName').value || "Clone_Net";
            const bQty = parseInt(document.getElementById('bCount').value) || 1;
            let names = [];
            for(let i=0; i<bQty; i++) names.push(bName);
            
            try {
                await fetch('/api/action', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({ action: 'ATTACK_BEACON', names: names })
                });
                document.getElementById('log').innerHTML += `> Spawning ${bQty} Beacons: ${bName}<br>`;
            } catch(e) { document.getElementById('log').innerHTML += "> HTTP Error<br>"; }
        }

        async function getScans() {
            try {
                const r = await fetch('/api/scans');
                const d = await r.json();
                if(d.status === "running") {
                    document.getElementById('scan-results').innerHTML = "Scan in progress... wait a moment."; return;
                }
                let html = "<b>FOUND NETWORKS:</b><br>";
                d.networks.forEach(n => {
                    // Inline Attack Button generated for EVERY network
                    html += `[${n.rssi}dBm] CH:${n.ch} | <b>${n.ssid}</b> <button class="attack inline-btn" onclick="startDeauth('${n.bssid}', ${n.ch}, '${n.ssid}')">DEAUTH</button><br>`;
                });
                document.getElementById('scan-results').innerHTML = html || "No networks found.";
            } catch(e) { document.getElementById('scan-results').innerHTML = "Failed to fetch scans."; }
        }

        function clearLog() { document.getElementById('log').innerHTML = ""; }
    </script>
</body>
</html>
)rawliteral";
// ==========================================


namespace DeviceConfig {
constexpr char kProjectName[] = "BruceForce";
constexpr char kApSsid[] = "BruceForce";
constexpr char kApPassword[] = "SOURAV14692";
constexpr char kAuthToken[] = "SOURAV14692";

const IPAddress kApIp(192, 168, 4, 1);
const IPAddress kGatewayIp(192, 168, 4, 1);
const IPAddress kSubnetMask(255, 255, 255, 0);

constexpr uint16_t kHttpPort = 80;
constexpr uint16_t kWsPort = 81;
constexpr uint32_t kSerialBaud = 115200;
constexpr uint32_t kStatsIntervalMs = 500;
constexpr uint32_t kHeartbeatIntervalMs = 30000;
constexpr uint32_t kDisplayRefreshMs = 1200;

constexpr size_t kMaxTrackedClients = 8;
constexpr size_t kMaxScanNetworks = 16;

constexpr int kScreenWidth = 128;
constexpr int kScreenHeight = 64;
constexpr int kOledResetPin = -1;
constexpr uint8_t kOledAddress = 0x3C;
constexpr uint8_t kI2cSdaPin = 21;
constexpr uint8_t kI2cSclPin = 22;

constexpr bool kAttackActionsEnabled = false;
}  // namespace DeviceConfig

struct ClientSession {
  uint32_t id = 0;
  bool active = false;
  bool authenticated = false;
};

struct RuntimeState {
  bool oledReady = false;
  bool scanRunning = false;
  bool safeMode = true;
  uint32_t wsClientCount = 0;
  uint32_t packetActivityProxy = 0;
  uint32_t blockedAttackRequests = 0;
  uint32_t lastScanNetworkCount = 0;
  uint8_t currentChannel = 1;
  unsigned long lastStatsAt = 0;
  unsigned long lastDisplayAt = 0;
  unsigned long lastHeartbeatAt = 0;
} runtime;

Adafruit_SSD1306 display(
    DeviceConfig::kScreenWidth,
    DeviceConfig::kScreenHeight,
    &Wire,
    DeviceConfig::kOledResetPin);

AsyncWebServer httpServer(DeviceConfig::kHttpPort);
AsyncWebServer wsServer(DeviceConfig::kWsPort);
AsyncWebSocket socketEndpoint("/ws");

ClientSession sessions[DeviceConfig::kMaxTrackedClients];

String getSoftApIpString() {
  return WiFi.softAPIP().toString();
}

String formatMac(const uint8_t* mac) {
  char buffer[18];
  snprintf(
      buffer,
      sizeof(buffer),
      "%02X:%02X:%02X:%02X:%02X:%02X",
      mac[0],
      mac[1],
      mac[2],
      mac[3],
      mac[4],
      mac[5]);
  return String(buffer);
}

ClientSession* findSession(const uint32_t clientId, const bool createIfMissing) {
  for (ClientSession& session : sessions) {
    if (session.active && session.id == clientId) {
      return &session;
    }
  }

  if (!createIfMissing) {
    return nullptr;
  }

  for (ClientSession& session : sessions) {
    if (!session.active) {
      session.active = true;
      session.authenticated = false;
      session.id = clientId;
      return &session;
    }
  }

  return nullptr;
}

void clearSession(const uint32_t clientId) {
  for (ClientSession& session : sessions) {
    if (session.active && session.id == clientId) {
      session = ClientSession{};
      return;
    }
  }
}

void addActivity(const uint32_t amount) {
  runtime.packetActivityProxy = min<uint32_t>(runtime.packetActivityProxy + amount, 999);
}

void decayActivity() {
  if (runtime.packetActivityProxy > 4) {
    runtime.packetActivityProxy -= 4;
  } else {
    runtime.packetActivityProxy = 0;
  }
}

void sendJsonToClient(AsyncWebSocketClient* client, const JsonDocument& document) {
  String payload;
  serializeJson(document, payload);
  client->text(payload);
}

void broadcastJson(const JsonDocument& document) {
  String payload;
  serializeJson(document, payload);
  socketEndpoint.textAll(payload);
}

void sendError(AsyncWebSocketClient* client, const char* code, const char* detail) {
  StaticJsonDocument<192> document;
  document["type"] = "error";
  document["code"] = code;
  document["detail"] = detail;
  sendJsonToClient(client, document);
}

void sendAck(AsyncWebSocketClient* client, const char* action, const char* status, const char* detail) {
  StaticJsonDocument<224> document;
  document["type"] = "ack";
  document["action"] = action;
  document["status"] = status;
  document["detail"] = detail;
  sendJsonToClient(client, document);
}

void printCredentialsToSerial() {
  Serial.println();
  Serial.println("========================================");
  Serial.println("BruceForce safe integration firmware");
  Serial.println("========================================");
  Serial.print("WiFi Access Point Name : ");
  Serial.println(DeviceConfig::kApSsid);
  Serial.print("WiFi Password          : ");
  Serial.println(DeviceConfig::kApPassword);
  Serial.print("Auth token             : ");
  Serial.println(DeviceConfig::kAuthToken);
  Serial.print("HTTP status URL        : http://");
  Serial.println(getSoftApIpString());
  Serial.print("JSON status URL        : http://");
  Serial.print(getSoftApIpString());
  Serial.println("/status");
  Serial.print("WebSocket URL          : ws://");
  Serial.print(getSoftApIpString());
  Serial.println(":81/ws");
  Serial.println("Manual hook file       : CustomRadioHooks.cpp");
  Serial.println("Attack commands        : controlled via custom hooks");
  Serial.println("========================================");
  Serial.println();
}

void renderDisplay() {
  if (!runtime.oledReady) {
    return;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("BruceForce");
  display.print("SSID:");
  display.println(DeviceConfig::kApSsid);
  display.print("PASS:");
  display.println(DeviceConfig::kApPassword);
  display.print("IP:");
  display.println(getSoftApIpString());
  display.print("WS:");
  display.print(DeviceConfig::kWsPort);
  display.print(" AP:");
  display.println(WiFi.softAPgetStationNum());
  display.print("Scan:");
  display.println(runtime.scanRunning ? "RUN" : "IDLE");
  display.print("Mode:");
  display.println("READY");
  display.display();
}

void initDisplay() {
  Wire.begin(DeviceConfig::kI2cSdaPin, DeviceConfig::kI2cSclPin);
  runtime.oledReady = display.begin(SSD1306_SWITCHCAPVCC, DeviceConfig::kOledAddress);

  if (runtime.oledReady) {
    Serial.println("OLED detected on I2C address 0x3C.");
    renderDisplay();
  } else {
    Serial.println("OLED not detected. Continuing with serial output only.");
  }
}

void handleRoot(AsyncWebServerRequest* request) {
  // SERVE THE EMBEDDED HTML FILE FROM FLASH MEMORY
  request->send_P(200, "text/html", index_html);
}

void handleStatus(AsyncWebServerRequest* request) {
  DynamicJsonDocument document(2048);
  
  document["device"] = DeviceConfig::kProjectName;
  document["ap_ssid"] = DeviceConfig::kApSsid;
  document["ap_password"] = DeviceConfig::kApPassword;
  document["auth_token"] = DeviceConfig::kAuthToken;
  document["ip"] = getSoftApIpString();
  document["http_port"] = DeviceConfig::kHttpPort;
  document["ws_port"] = DeviceConfig::kWsPort;
  document["ap_clients"] = WiFi.softAPgetStationNum();
  document["ws_clients"] = runtime.wsClientCount;
  document["scan_running"] = runtime.scanRunning;
  document["safe_mode"] = runtime.safeMode;
  document["attack_actions_enabled"] = true;
  document["oled_ready"] = runtime.oledReady;
  document["channel"] = runtime.currentChannel;

  appendCustomRadioStatus(document.as<JsonObject>());

  String payload;
  serializeJson(document, payload);
  request->send(200, "application/json", payload);
}

void handleOptions(AsyncWebServerRequest* request) {
  request->send(204);
}

void broadcastStats() {
  StaticJsonDocument<512> document;
  document["type"] = "stats";
  document["pkts"] = runtime.packetActivityProxy;
  document["deauths"] = 0;
  document["channel"] = runtime.currentChannel;
  document["safe_mode"] = runtime.safeMode;
  JsonObject manual = document.createNestedObject("manual");
  appendCustomRadioStats(manual);
  broadcastJson(document);
}

void broadcastScanResults(const int networkCount) {
  StaticJsonDocument<3072> document;
  document["type"] = "scan";
  JsonArray networks = document.createNestedArray("networks");

  const int limit = min(networkCount, static_cast<int>(DeviceConfig::kMaxScanNetworks));
  for (int index = 0; index < limit; ++index) {
    JsonObject network = networks.createNestedObject();
    network["ssid"] = WiFi.SSID(index);
    network["bssid"] = WiFi.BSSIDstr(index);
    network["rssi"] = WiFi.RSSI(index);
    network["ch"] = WiFi.channel(index);
  }

  broadcastJson(document);
}

void startAsyncScan() {
  if (runtime.scanRunning) {
    return;
  }

  WiFi.scanDelete();
  runtime.scanRunning = true;
  runtime.currentChannel = WiFi.channel();
  addActivity(50);
  WiFi.scanNetworks(true, true);
  Serial.println("Async network scan started.");
  renderDisplay();
}

void stopOperations() {
  runtime.scanRunning = false;
  runtime.lastScanNetworkCount = 0;
  WiFi.scanDelete();
  stopCustomRadioHooks();
  renderDisplay();
}

void handleCommand(AsyncWebSocketClient* client, JsonDocument& document) {
  const char* action = document["action"] | "";
  if (strlen(action) == 0) {
    sendError(client, "missing_action", "Command packet did not include an action field.");
    return;
  }

  addActivity(18);

  if (strcmp(action, "SCAN_START") == 0) {
    startAsyncScan();
    sendAck(client, action, "accepted", "Asynchronous scan started.");
    return;
  }

  if (strcmp(action, "STOP") == 0) {
    stopOperations();
    sendAck(client, action, "accepted", "Active tasks were stopped.");
    return;
  }

  DynamicJsonDocument customResponse(1024);
  if (handleCustomRadioAction(document, customResponse, Serial)) {
    sendJsonToClient(client, customResponse);
    return;
  }

  sendError(client, "unknown_action", "Command action is not supported.");
}

void handleSocketMessage(AsyncWebSocketClient* client, const uint8_t* data, const size_t length) {
  StaticJsonDocument<512> document;
  const DeserializationError error = deserializeJson(document, data, length);
  if (error) {
    sendError(client, "invalid_json", "Could not parse JSON command packet.");
    return;
  }

  ClientSession* session = findSession(client->id(), true);
  if (session == nullptr) {
    sendError(client, "session_limit", "No free session slots are available.");
    client->close();
    return;
  }

  if (!session->authenticated) {
    const char* token = document["auth_token"] | "";
    if (strcmp(token, DeviceConfig::kAuthToken) != 0) {
      sendError(client, "auth_failed", "auth_token is missing or invalid.");
      client->close();
      return;
    }

    session->authenticated = true;
    sendAck(client, "AUTH", "ok", "WebSocket session authenticated.");
  }

  handleCommand(client, document);
}

void handleSocketEvent(
    AsyncWebSocket* server,
    AsyncWebSocketClient* client,
    AwsEventType type,
    void* arg,
    uint8_t* data,
    size_t len) {
  (void)server;

  if (type == WS_EVT_CONNECT) {
    findSession(client->id(), true);
    runtime.wsClientCount++;

    StaticJsonDocument<224> document;
    document["type"] = "hello";
    document["device"] = DeviceConfig::kProjectName;
    document["auth_required"] = true;
    document["safe_mode"] = runtime.safeMode;
    document["ws_port"] = DeviceConfig::kWsPort;
    sendJsonToClient(client, document);
    renderDisplay();
    return;
  }

  if (type == WS_EVT_DISCONNECT) {
    clearSession(client->id());
    if (runtime.wsClientCount > 0) {
      runtime.wsClientCount--;
    }
    renderDisplay();
    return;
  }

  if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = reinterpret_cast<AwsFrameInfo*>(arg);
    if (info == nullptr || !info->final || info->index != 0 || info->opcode != WS_TEXT) {
      sendError(client, "frame_unsupported", "Only complete text JSON frames are supported.");
      return;
    }

    handleSocketMessage(client, data, len);
  }
}

void registerCors() {
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
}

void registerHttpRoutes() {
  httpServer.on("/", HTTP_GET, handleRoot);
  httpServer.on("/", HTTP_OPTIONS, handleOptions);
  httpServer.on("/status", HTTP_GET, handleStatus);
  httpServer.on("/status", HTTP_OPTIONS, handleOptions);

  // NEW ROUTE: Send the scan results to the Lite UI
  httpServer.on("/api/scans", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(3072);
    int scanCount = WiFi.scanComplete();
    
    if (scanCount == WIFI_SCAN_RUNNING) {
      doc["status"] = "running";
      doc.createNestedArray("networks");
    } else {
      doc["status"] = "complete";
      JsonArray networks = doc.createNestedArray("networks");
      int count = max(0, scanCount);
      for (int i = 0; i < min(count, 16); ++i) {
        JsonObject net = networks.createNestedObject();
        net["ssid"] = WiFi.SSID(i);
        net["bssid"] = WiFi.BSSIDstr(i);
        net["rssi"] = WiFi.RSSI(i);
        net["ch"] = WiFi.channel(i);
      }
    }
    String out;
    serializeJson(doc, out);
    request->send(200, "application/json", out);
  });

  httpServer.on("/api/action", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);
    DynamicJsonDocument response(1024);
    doc["action"] = request->arg("action");
    handleCustomRadioAction(doc, response, Serial);
    String out;
    serializeJson(response, out);
    request->send(200, "application/json", out);
  });

  httpServer.on("/api/action", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, 
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      DynamicJsonDocument doc(2048);
      DynamicJsonDocument response(1024);
      
      DeserializationError error = deserializeJson(doc, data, len);
      if (error) {
          request->send(400, "application/json", "{\"error\":\"invalid_json\"}");
          return;
      }

      // Check if it's the SCAN command passed via HTTP POST
      if (doc["action"] == "SCAN_START") {
        startAsyncScan();
        response["detail"] = "Scan started";
      } else {
        // Fallback to custom radio hook actions (Beacon, Deauth, etc)
        handleCustomRadioAction(doc, response, Serial);
      }
      
      String out;
      serializeJson(response, out);
      request->send(200, "application/json", out);
  });

  httpServer.begin();

  socketEndpoint.onEvent(handleSocketEvent);
  wsServer.addHandler(&socketEndpoint);
  wsServer.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "application/json", "{\"error\":\"not_found\"}");
  });
  wsServer.begin();
}

void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_AP_START:
      Serial.println("WiFi access point started.");
      break;

    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      Serial.print("Station connected: ");
      Serial.println(formatMac(info.wifi_ap_staconnected.mac));
      addActivity(14);
      renderDisplay();
      break;

    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      Serial.print("Station disconnected: ");
      Serial.println(formatMac(info.wifi_ap_stadisconnected.mac));
      renderDisplay();
      break;

    default:
      break;
  }
}

void startSoftAp() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.onEvent(onWiFiEvent);
  WiFi.softAPConfig(
      DeviceConfig::kApIp,
      DeviceConfig::kGatewayIp,
      DeviceConfig::kSubnetMask);

  const bool apStarted = WiFi.softAP(DeviceConfig::kApSsid, DeviceConfig::kApPassword, 1, false, 4);
  if (!apStarted) {
    Serial.println("Failed to start WiFi access point.");
    ESP.restart();
  }

  runtime.currentChannel = WiFi.channel();
}

void pollScanResults() {
  if (!runtime.scanRunning) {
    return;
  }

  const int scanStatus = WiFi.scanComplete();
  if (scanStatus == WIFI_SCAN_RUNNING) {
    return;
  }

  runtime.scanRunning = false;
  if (scanStatus < 0) {
    Serial.println("Async scan failed or returned no data.");
    WiFi.scanDelete();
    renderDisplay();
    return;
  }

  runtime.lastScanNetworkCount = static_cast<uint32_t>(scanStatus);
  addActivity(40 + (runtime.lastScanNetworkCount * 8));
  broadcastScanResults(scanStatus);
  Serial.print("Scan finished. Networks found: ");
  Serial.println(scanStatus);
  WiFi.scanDelete();
  renderDisplay();
}

void maybeBroadcastStats() {
  const unsigned long now = millis();
  if (now - runtime.lastStatsAt < DeviceConfig::kStatsIntervalMs) {
    return;
  }

  runtime.lastStatsAt = now;
  runtime.currentChannel = WiFi.channel();
  broadcastStats();
}

void maybeRefreshDisplay() {
  const unsigned long now = millis();
  if (now - runtime.lastDisplayAt < DeviceConfig::kDisplayRefreshMs) {
    return;
  }

  runtime.lastDisplayAt = now;
  renderDisplay();
}

void maybeLogHeartbeat() {
  const unsigned long now = millis();
  if (now - runtime.lastHeartbeatAt < DeviceConfig::kHeartbeatIntervalMs) {
    return;
  }

  runtime.lastHeartbeatAt = now;
  Serial.print("Heartbeat | IP: ");
  Serial.print(getSoftApIpString());
  Serial.print(" | AP clients: ");
  Serial.print(WiFi.softAPgetStationNum());
  Serial.print(" | WS clients: ");
  Serial.print(runtime.wsClientCount);
  Serial.print(" | Scan: ");
  Serial.println(runtime.scanRunning ? "RUNNING" : "IDLE");
}

void setup() {
  Serial.begin(DeviceConfig::kSerialBaud);
  Serial.println();
  Serial.println("Booting BruceForce firmware...");

  startSoftAp();
  initDisplay();
  initCustomRadioHooks(Serial);
  registerCors();
  registerHttpRoutes();
  printCredentialsToSerial();
  renderDisplay();
}

void loop() {
  socketEndpoint.cleanupClients();
  pollScanResults();
  tickCustomRadioHooks();
  maybeBroadcastStats();
  maybeRefreshDisplay();
  maybeLogHeartbeat();
  decayActivity();
}
