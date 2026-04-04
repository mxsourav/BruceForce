#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "CustomRadioHooks.h"

namespace DeviceConfig {
constexpr char kProjectName[] = "DeautherNet";
constexpr char kApSsid[] = "DeautherNet";
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
  Serial.println("DeautherNet safe integration firmware");
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
  Serial.println("Attack commands        : disabled in this firmware build");
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
  display.println("DeautherNet");
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
  display.println("SAFE");
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
  AsyncResponseStream* response = request->beginResponseStream("text/html");
  response->print("<!doctype html><html><head><meta charset='utf-8'>");
  response->print("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  response->print("<title>DeautherNet</title><style>");
  response->print("body{font-family:Arial,sans-serif;background:#121417;color:#f3f5f8;padding:24px;}");
  response->print(".card{max-width:620px;margin:auto;padding:24px;border-radius:18px;background:#1b1e23;");
  response->print("box-shadow:0 16px 34px rgba(0,0,0,.34);}code{color:#8ec6ff;}strong{color:#fff;}");
  response->print("</style></head><body><div class='card'>");
  response->print("<h1>DeautherNet</h1>");
  response->print("<p>Safe integration build is active.</p>");
  response->print("<p><strong>SSID:</strong> ");
  response->print(DeviceConfig::kApSsid);
  response->print("</p><p><strong>Password:</strong> ");
  response->print(DeviceConfig::kApPassword);
  response->print("</p><p><strong>IP:</strong> ");
  response->print(getSoftApIpString());
  response->print("</p><p><strong>WebSocket:</strong> <code>ws://");
  response->print(getSoftApIpString());
  response->print(":81/ws</code></p>");
  response->print("<p><strong>Manual hooks:</strong> <code>CustomRadioHooks.cpp</code></p>");
  response->print("<p><strong>Safe mode:</strong> attack actions are intentionally blocked in this build.</p>");
  response->print("</div></body></html>");
  request->send(response);
}

void handleStatus(AsyncWebServerRequest* request) {
  StaticJsonDocument<512> document;
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
  document["attack_actions_enabled"] = DeviceConfig::kAttackActionsEnabled;
  document["oled_ready"] = runtime.oledReady;
  document["channel"] = runtime.currentChannel;
  JsonObject manualHooks = document.createNestedObject("manual_hooks");
  appendCustomRadioStatus(manualHooks);

  String payload;
  serializeJson(document, payload);
  request->send(200, "application/json", payload);
}

void handleOptions(AsyncWebServerRequest* request) {
  request->send(204);
}

void broadcastStats() {
  StaticJsonDocument<256> document;
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
    sendAck(client, action, "accepted", "Active safe-mode tasks were stopped.");
    return;
  }

  StaticJsonDocument<256> customResponse;
  if (handleCustomRadioAction(document, customResponse, Serial)) {
    const char* responseType = customResponse["type"] | "";
    if (strcmp(responseType, "error") == 0) {
      runtime.blockedAttackRequests++;
    }
    sendJsonToClient(client, customResponse);
    return;
  }

  sendError(client, "unknown_action", "Command action is not supported.");
}

void handleSocketMessage(AsyncWebSocketClient* client, const uint8_t* data, const size_t length) {
  StaticJsonDocument<256> document;
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
  Serial.print(runtime.scanRunning ? "RUNNING" : "IDLE");
  Serial.print(" | Safe mode: ");
  Serial.println(runtime.safeMode ? "ON" : "OFF");
}

void setup() {
  Serial.begin(DeviceConfig::kSerialBaud);
  Serial.println();
  Serial.println("Booting DeautherNet safe integration firmware...");

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
