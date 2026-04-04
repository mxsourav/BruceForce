#ifndef CUSTOM_RADIO_HOOKS_H
#define CUSTOM_RADIO_HOOKS_H

#include <Arduino.h>
#include <ArduinoJson.h>

void initCustomRadioHooks(Stream& logSink);
void tickCustomRadioHooks();
void stopCustomRadioHooks();
bool handleCustomRadioAction(JsonDocument& request, JsonDocument& response, Stream& logSink);
void appendCustomRadioStatus(JsonObject statusObject);
void appendCustomRadioStats(JsonObject statsObject);

#endif
