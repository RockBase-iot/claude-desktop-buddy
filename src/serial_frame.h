#pragma once
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static const char SERIAL_FRAME_PREFIX[] = "@CDB ";

inline const char* serialFramePayload(const char* line) {
  if (!line) return nullptr;
  size_t prefixLen = strlen(SERIAL_FRAME_PREFIX);
  if (strncmp(line, SERIAL_FRAME_PREFIX, prefixLen) == 0) {
    const char* payload = line + prefixLen;
    return payload[0] == '{' ? payload : nullptr;
  }
  return line[0] == '{' ? line : nullptr;
}

inline size_t serialFrameBuild(char* out, size_t outLen, const char* json) {
  if (!out || outLen == 0 || !json) return 0;
  int n = snprintf(out, outLen, "%s%s\n", SERIAL_FRAME_PREFIX, json);
  if (n < 0) {
    out[0] = 0;
    return 0;
  }
  return (size_t)n < outLen ? (size_t)n : outLen - 1;
}
