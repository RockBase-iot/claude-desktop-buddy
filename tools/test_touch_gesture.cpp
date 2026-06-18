#include <assert.h>
#include <stdint.h>
#include "touch_gesture.h"

static void single_tap_emits_a_after_double_tap_window() {
  TouchGestureButton g;

  g.update(false, 0);
  g.update(true, 10);
  assert(g.aIsPressed());

  g.update(false, 60);
  assert(!g.aWasReleased());
  assert(!g.bWasPressed());

  g.update(false, 60 + TouchGestureButton::DOUBLE_TAP_MS - 1);
  assert(!g.aWasReleased());

  g.update(false, 60 + TouchGestureButton::DOUBLE_TAP_MS);
  assert(g.aWasReleased());
  assert(!g.bWasPressed());
}

static void double_tap_emits_one_b_event_and_no_a_event() {
  TouchGestureButton g;

  g.update(true, 10);
  g.update(false, 60);
  assert(!g.aWasReleased());

  g.update(true, 160);
  assert(!g.aIsPressed());
  assert(g.bIsPressed());

  g.update(false, 210);
  assert(!g.aWasReleased());
  assert(g.bWasPressed());

  g.update(false, 60 + TouchGestureButton::DOUBLE_TAP_MS + 20);
  assert(!g.aWasReleased());
  assert(!g.bWasPressed());
}

static void long_press_keeps_a_pressed_for_and_releases_immediately() {
  TouchGestureButton g;

  g.update(true, 100);
  g.update(true, 100 + TouchGestureButton::LONG_PRESS_MS);
  assert(g.aIsPressed());
  assert(g.aPressedFor(TouchGestureButton::LONG_PRESS_MS, 100 + TouchGestureButton::LONG_PRESS_MS));

  g.update(false, 100 + TouchGestureButton::LONG_PRESS_MS + 20);
  assert(g.aWasReleased());
  assert(!g.bWasPressed());
}

static void tap_after_double_tap_window_starts_a_new_single_tap() {
  TouchGestureButton g;

  g.update(true, 10);
  g.update(false, 60);

  g.update(true, 60 + TouchGestureButton::DOUBLE_TAP_MS);
  assert(g.aWasReleased());
  assert(g.aIsPressed());
  assert(!g.bWasPressed());
}

int main() {
  single_tap_emits_a_after_double_tap_window();
  double_tap_emits_one_b_event_and_no_a_event();
  long_press_keeps_a_pressed_for_and_releases_immediately();
  tap_after_double_tap_window_starts_a_new_single_tap();
  return 0;
}
