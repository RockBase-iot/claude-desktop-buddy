#pragma once
#include <Arduino.h>
#include <stddef.h>
#include <string.h>
#include "ble_bridge.h"
#include "serial_frame.h"

inline void transportWriteJson(const char* json) {
  if (!json) return;

#ifdef NMTV154_BOARD
  Serial.print(SERIAL_FRAME_PREFIX);
  Serial.println(json);
#else
  Serial.println(json);
#endif

  size_t n = strlen(json);
  bleWrite((const uint8_t*)json, n);
  bleWrite((const uint8_t*)"\n", 1);
}

inline void transportWriteJsonBytes(const char* json, size_t len) {
  if (!json) return;

#ifdef NMTV154_BOARD
  Serial.print(SERIAL_FRAME_PREFIX);
  Serial.write((const uint8_t*)json, len);
  if (len == 0 || json[len - 1] != '\n') Serial.write('\n');
#else
  Serial.write((const uint8_t*)json, len);
#endif

  bleWrite((const uint8_t*)json, len);
}
