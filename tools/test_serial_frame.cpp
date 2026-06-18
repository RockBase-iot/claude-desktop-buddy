#include <assert.h>
#include <string.h>
#include "serial_frame.h"

static void framed_lines_unwrap_to_json() {
  const char* json = serialFramePayload("@CDB {\"cmd\":\"status\"}");
  assert(json != nullptr);
  assert(strcmp(json, "{\"cmd\":\"status\"}") == 0);
}

static void raw_json_lines_stay_supported() {
  const char* json = serialFramePayload("{\"cmd\":\"status\"}");
  assert(json != nullptr);
  assert(strcmp(json, "{\"cmd\":\"status\"}") == 0);
}

static void logs_are_not_protocol_payloads() {
  assert(serialFramePayload("[ble] connected") == nullptr);
  assert(serialFramePayload("ets Jun  8 2016 00:22:57") == nullptr);
}

static void nmtv154_serial_output_is_framed() {
  char out[64];
  size_t n = serialFrameBuild(out, sizeof(out), "{\"ack\":\"status\"}");
  assert(n == strlen("@CDB {\"ack\":\"status\"}\n"));
  assert(strcmp(out, "@CDB {\"ack\":\"status\"}\n") == 0);
}

int main() {
  framed_lines_unwrap_to_json();
  raw_json_lines_stay_supported();
  logs_are_not_protocol_payloads();
  nmtv154_serial_output_is_framed();
  return 0;
}
