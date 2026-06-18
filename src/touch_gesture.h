#pragma once
#include <stdint.h>

class TouchGestureButton {
public:
  static const uint32_t DOUBLE_TAP_MS = 280;
  static const uint32_t LONG_PRESS_MS = 600;

  void update(bool touched, uint32_t now) {
    _aRelease = false;
    _bRelease = false;

    if (_pendingSingle && (uint32_t)(now - _pendingReleaseMs) >= DOUBLE_TAP_MS) {
      _aRelease = true;
      _pendingSingle = false;
    }

    bool rising = touched && !_rawDown;
    bool falling = !touched && _rawDown;

    if (rising) {
      _rawDown = true;
      if (_pendingSingle && (uint32_t)(now - _pendingReleaseMs) < DOUBLE_TAP_MS) {
        _pendingSingle = false;
        _doubleDown = true;
        _aDown = false;
        _aDownMs = 0;
        return;
      }

      _aDown = true;
      _aDownMs = now;
      return;
    }

    if (falling) {
      _rawDown = false;
      if (_doubleDown) {
        _doubleDown = false;
        _bRelease = true;
        return;
      }

      if (_aDown) {
        if ((uint32_t)(now - _aDownMs) >= LONG_PRESS_MS) {
          _aRelease = true;
        } else {
          _pendingSingle = true;
          _pendingReleaseMs = now;
        }
        _aDown = false;
        _aDownMs = 0;
      }
    }
  }

  bool aIsPressed() const { return _aDown; }
  bool aWasReleased() const { return _aRelease; }
  bool aPressedFor(uint32_t ms, uint32_t now) const {
    return _aDown && _aDownMs && (uint32_t)(now - _aDownMs) >= ms;
  }
  bool bIsPressed() const { return _doubleDown; }
  bool bWasPressed() const { return _bRelease; }

private:
  bool _rawDown = false;
  bool _aDown = false;
  bool _aRelease = false;
  uint32_t _aDownMs = 0;

  bool _pendingSingle = false;
  uint32_t _pendingReleaseMs = 0;

  bool _doubleDown = false;
  bool _bRelease = false;
};
