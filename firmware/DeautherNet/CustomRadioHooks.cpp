#include "CustomRadioHooks.h"
#include "WifiBeacon.hpp"
#include <vector>
#include <string>

namespace {

struct CustomRadioState {
  bool customTaskActive = false;
  bool deauthHookImplemented = false;
  bool beaconHookImplemented = false;
  bool snifferHookImplemented = false;
  uint32_t customPacketCounter = 0;
  uint32_t customEventCounter = 0;
  uint8_t channel = 1;
  struct {
    bool active = false;
    uint32_t frames = 0;
    uint32_t managementFrames = 0;
    uint32_t dataFrames = 0;
    uint32_t deauthFrames = 0;
    uint32_t authFrames = 0;
    uint32_t assocFrames = 0;
    uint32_t disassocFrames = 0;
    int lastRssi = 0;
    uint32_t stationsTracked = 0;
  } sniffer;
};

CustomRadioState customState;
Stream* logStream = nullptr;

// THE FIX: We use a pointer here so we can feed it the list of network names later!
static WifiBeacon* beaconEngine = nullptr; 

void buildStubResponse(JsonDocument& response, const char* action, const char* detail);

void logLine(const char* message) {
  if (logStream != nullptr) {
    logStream->println(message);
  }
}

// ============================================================
// MANUAL LINK AREA
// ============================================================

bool handleBeaconModuleAction(JsonDocument& request, JsonDocument& response) {
  const char* action = request["action"] | "";

  if (strcmp(action, "ATTACK_BEACON") != 0) {
    return false;
  }

  // --- MANUAL BEACON WIRING: START ---
  
  // 1. Give it a list of fake networks to broadcast if it doesn't exist yet
  if (beaconEngine == nullptr) {
    std::vector<std::string> myFakeNetworks = {
      "Free_Starbucks_WiFi",
      "FBI_Surveillance_Van_4",
      "Tell_My_WiFi_Love_Her"
    };
    beaconEngine = new WifiBeacon(myFakeNetworks);
  }

  // 2. Open the radio and update the dashboard state
  if (beaconEngine->open()) {
    customState.customTaskActive = true;
    customState.beaconHookImplemented = true;
    logLine("Manual Beacon Engine: STARTED");

    // 3. Send a success message back to the Web UI
    response.clear();
    response["type"] = "success";
    response["action"] = action;
    response["detail"] = "Beacon engine successfully fired up.";
  } else {
    buildStubResponse(response, action, "Failed to open beacon radio.");
  }

  return true;
}

bool handleSnifferModuleAction(JsonDocument& request, JsonDocument& response) {
  const char* action = request["action"] | "";

  if (strcmp(action, "SNIFFER_START") != 0 && strcmp(action, "SNIFFER_STOP") != 0) {
    return false;
  }

  // USER LINK POINT: Call into your wifi_sniff.cpp module here.

  buildStubResponse(
      response,
      action,
      "Sniffer hook placeholder. Link your own wifi_sniff.cpp calls in handleSnifferModuleAction().");
  return true;
}

bool handleRouteModuleAction(JsonDocument& request, JsonDocument& response) {
  const char* action = request["action"] | "";

  if (strcmp(action, "ATTACK_DEAUTH") != 0 && strncmp(action, "CUSTOM_", 7) != 0) {
    return false;
  }

  // USER LINK POINT: Call into your wifi_route.cpp module here.

  buildStubResponse(
      response,
      action,
      "Route hook placeholder. Link your own wifi_route.cpp calls in handleRouteModuleAction().");
  return true;
}

void tickBeaconModule() {
  // --- MANUAL BEACON WIRING: TICK ---
  if (customState.customTaskActive && customState.beaconHookImplemented && beaconEngine != nullptr) {
    beaconEngine->sendNextBeacon(); // Send a packet
    customState.customPacketCounter++; // Update the UI counter
  }
}

void tickSnifferModule() {
  // USER LINK POINT: Refresh counters from your wifi_sniff.cpp module here.
}

void tickRouteModule() {
  // USER LINK POINT: Refresh state from your wifi_route.cpp module here.
}

void stopBeaconModule() {
  // --- MANUAL BEACON WIRING: STOP ---
  if (beaconEngine != nullptr) {
    beaconEngine->close();
    delete beaconEngine;
    beaconEngine = nullptr;
    logLine("Manual Beacon Engine: STOPPED");
  }
  customState.beaconHookImplemented = false;
}

void stopSnifferModule() {
  customState.sniffer.active = false;
}

void stopRouteModule() {
  // USER LINK POINT: Stop route-related work here.
}

bool isCustomAction(const char* action) {
  return strcmp(action, "ATTACK_DEAUTH") == 0 ||
         strcmp(action, "ATTACK_BEACON") == 0 ||
         strcmp(action, "SNIFFER_START") == 0 ||
         strcmp(action, "SNIFFER_STOP") == 0 ||
         strncmp(action, "CUSTOM_", 7) == 0;
}

void buildStubResponse(JsonDocument& response, const char* action, const char* detail) {
  response.clear();
  response["type"] = "error";
  response["action"] = action;
  response["code"] = "manual_hook_empty";
  response["detail"] = detail;
  response["extension_file"] = "CustomRadioHooks.cpp";
}

}  // namespace

void initCustomRadioHooks(Stream& logSink) {
  logStream = &logSink;
  logLine("CustomRadioHooks ready. Edit CustomRadioHooks.cpp to add your own board-specific modules.");
}

void tickCustomRadioHooks() {
  tickBeaconModule();
  tickSnifferModule();
  tickRouteModule();
}

void stopCustomRadioHooks() {
  customState.customTaskActive = false;
  stopBeaconModule();
  stopSnifferModule();
  stopRouteModule();
}

bool handleCustomRadioAction(JsonDocument& request, JsonDocument& response, Stream& logSink) {
  logStream = &logSink;

  const char* action = request["action"] | "";
  if (!isCustomAction(action)) {
    return false;
  }

  if (handleBeaconModuleAction(request, response)) {
    return true;
  }

  if (handleSnifferModuleAction(request, response)) {
    return true;
  }

  if (handleRouteModuleAction(request, response)) {
    return true;
  }

  return false;
}

void appendCustomRadioStatus(JsonObject statusObject) {
  statusObject["file"] = "CustomRadioHooks.cpp";
  statusObject["custom_task_active"] = customState.customTaskActive;
  statusObject["deauth_hook_implemented"] = customState.deauthHookImplemented;
  statusObject["beacon_hook_implemented"] = customState.beaconHookImplemented;
  statusObject["sniffer_hook_implemented"] = customState.snifferHookImplemented;
  statusObject["custom_packets"] = customState.customPacketCounter;
  statusObject["custom_events"] = customState.customEventCounter;
  statusObject["channel"] = customState.channel;
  JsonObject sniffer = statusObject.createNestedObject("sniffer");
  sniffer["active"] = customState.sniffer.active;
  sniffer["frames"] = customState.sniffer.frames;
  sniffer["management"] = customState.sniffer.managementFrames;
  sniffer["data"] = customState.sniffer.dataFrames;
  sniffer["deauth"] = customState.sniffer.deauthFrames;
  sniffer["auth"] = customState.sniffer.authFrames;
  sniffer["assoc"] = customState.sniffer.assocFrames;
  sniffer["disassoc"] = customState.sniffer.disassocFrames;
  sniffer["last_rssi"] = customState.sniffer.lastRssi;
  sniffer["stations_tracked"] = customState.sniffer.stationsTracked;
}

void appendCustomRadioStats(JsonObject statsObject) {
  statsObject["custom_packets"] = customState.customPacketCounter;
  statsObject["custom_events"] = customState.customEventCounter;
  statsObject["channel"] = customState.channel;
  statsObject["custom_task_active"] = customState.customTaskActive;
  JsonObject sniffer = statsObject.createNestedObject("sniffer");
  sniffer["active"] = customState.sniffer.active;
  sniffer["frames"] = customState.sniffer.frames;
  sniffer["management"] = customState.sniffer.managementFrames;
  sniffer["data"] = customState.sniffer.dataFrames;
  sniffer["deauth"] = customState.sniffer.deauthFrames;
  sniffer["auth"] = customState.sniffer.authFrames;
  sniffer["assoc"] = customState.sniffer.assocFrames;
  sniffer["disassoc"] = customState.sniffer.disassocFrames;
  sniffer["last_rssi"] = customState.sniffer.lastRssi;
  sniffer["stations_tracked"] = customState.sniffer.stationsTracked;
}
