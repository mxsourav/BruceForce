#include "CustomRadioHooks.h"
#include "WifiBeacon.hpp"
#include "WifiDeauth.hpp"
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

static WifiBeacon* beaconEngine = nullptr; 
static WifiDeauth* deauthEngine = nullptr;

// --- HELPER: LOGGING ---
void logLine(const char* message) {
  if (logStream != nullptr) {
    logStream->println(message);
  }
}

// --- HELPER: STOP FUNCTIONS (Moved to top to fix "not declared" error) ---

void stopBeaconModule() {
  if (beaconEngine != nullptr) {
    beaconEngine->close();
    delete beaconEngine;
    beaconEngine = nullptr;
    logLine("Beacon Engine: STOPPED");
  }
  customState.beaconHookImplemented = false;
  if (!customState.deauthHookImplemented) {
      customState.customTaskActive = false;
  }
}

void stopRouteModule() {
  if (deauthEngine != nullptr) {
    deauthEngine->close();
    delete deauthEngine;
    deauthEngine = nullptr;
    logLine("Deauth Engine: STOPPED");
  }
  customState.deauthHookImplemented = false;
  if (!customState.beaconHookImplemented) {
      customState.customTaskActive = false;
  }
}

void buildStubResponse(JsonDocument& response, const char* action, const char* detail) {
  response.clear();
  response["type"] = "error";
  response["action"] = action;
  response["code"] = "hardware_failure";
  response["detail"] = detail;
}

// --- MODULE HANDLERS ---

bool handleBeaconModuleAction(JsonDocument& request, JsonDocument& response) {
  const char* action = request["action"] | "";

  if (strcmp(action, "STOP_BEACON") == 0) {
      stopBeaconModule();
      response.clear();
      response["type"] = "success";
      response["detail"] = "Beacon spam stopped.";
      return true;
  }

  if (strcmp(action, "ATTACK_BEACON") != 0 && strcmp(action, "EMULATE_BEACON_STRESS_TEST") != 0) {
    return false;
  }

  logLine("Configuring Beacon Engine...");
  std::vector<std::string> userNetworks;
  JsonArray names = request["names"].as<JsonArray>();

  if (names.size() > 0) {
      for (JsonVariant v : names) {
          userNetworks.push_back(v.as<std::string>());
      }
  } else {
      userNetworks.push_back("BruceForce_Default");
  }

  stopBeaconModule(); // Now the compiler knows where this is!

  beaconEngine = new WifiBeacon(userNetworks);
  if (beaconEngine->open()) {
    customState.customTaskActive = true;
    customState.beaconHookImplemented = true;
    logLine("Beacon Engine: ACTIVE");
    response.clear();
    response["type"] = "success";
    response["detail"] = "Broadcasting " + String(userNetworks.size()) + " networks.";
  } else {
    buildStubResponse(response, action, "Hardware error: Could not open radio.");
  }
  return true;
}

bool handleRouteModuleAction(JsonDocument& request, JsonDocument& response) {
  const char* action = request["action"] | "";
  if (strcmp(action, "ATTACK_DEAUTH") != 0) return false;

  logLine("Configuring Deauth Engine...");
  std::string target = "Broadcast";
  JsonArray names = request["names"].as<JsonArray>();
  if (names.size() > 0) target = names[0].as<std::string>();

  stopRouteModule(); 

  deauthEngine = new WifiDeauth(target);
  if (deauthEngine->open()) {
      customState.customTaskActive = true;
      customState.deauthHookImplemented = true;
      logLine(("Deauth Engine: ARMED against " + target).c_str());
      response.clear();
      response["type"] = "success";
      response["detail"] = "Deauth module armed.";
  } else {
      buildStubResponse(response, action, "Hardware error: Could not open radio.");
  }
  return true;
}

bool handleSnifferModuleAction(JsonDocument& request, JsonDocument& response) {
  const char* action = request["action"] | "";
  if (strcmp(action, "SNIFFER_START") != 0 && strcmp(action, "SNIFFER_STOP") != 0) return false;
  buildStubResponse(response, action, "Sniffer not yet linked.");
  return true;
}

bool isCustomAction(const char* action) {
  return strcmp(action, "ATTACK_DEAUTH") == 0 ||
         strcmp(action, "ATTACK_BEACON") == 0 ||
         strcmp(action, "STOP_BEACON") == 0 ||
         strcmp(action, "SNIFFER_START") == 0 ||
         strncmp(action, "CUSTOM_", 7) == 0;
}

}  // namespace

// --- PUBLIC API ---

void initCustomRadioHooks(Stream& logSink) {
  logStream = &logSink;
  logLine("CustomRadioHooks initialized.");
}

void tickCustomRadioHooks() {
  if (customState.customTaskActive) {
    if (customState.beaconHookImplemented && beaconEngine != nullptr) {
        beaconEngine->sendNextBeacon(); 
        customState.customPacketCounter++; 
    }
    if (customState.deauthHookImplemented && deauthEngine != nullptr) {
        deauthEngine->sendDeauth(); 
        customState.customPacketCounter++; 
    }
  }
}

void stopCustomRadioHooks() {
  stopBeaconModule();
  stopRouteModule();
}

bool handleCustomRadioAction(JsonDocument& request, JsonDocument& response, Stream& logSink) {
  logStream = &logSink;
  const char* action = request["action"] | "";
  if (!isCustomAction(action)) return false;

  if (handleBeaconModuleAction(request, response)) return true;
  if (handleSnifferModuleAction(request, response)) return true;
  if (handleRouteModuleAction(request, response)) return true;

  return false;
}

void appendCustomRadioStatus(JsonObject statusObject) {
  statusObject["custom_task_active"] = customState.customTaskActive;
  statusObject["beacon_active"] = customState.beaconHookImplemented;
  statusObject["deauth_active"] = customState.deauthHookImplemented;
  statusObject["packets"] = customState.customPacketCounter;
}

void appendCustomRadioStats(JsonObject statsObject) {
  statsObject["custom_packets"] = customState.customPacketCounter;
}
