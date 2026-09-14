#include "limit_switch.h"

LimitSwitch::LimitSwitch(uint8_t pin, bool activeLow)
    : _pin(pin), _activeLow(activeLow),
      _triggered(false), _lastTriggered(false),
      _justTriggered(false), _justReleased(false),
      _rawState(false), _lastReading(false),
      _lastDebounceTime(0) {}

void LimitSwitch::begin() {
    if (_activeLow) {
        pinMode(_pin, INPUT_PULLUP);
    } else {
        pinMode(_pin, INPUT_PULLDOWN);
    }
}

void LimitSwitch::update() {
    bool reading;
    if (_activeLow) {
        reading = (digitalRead(_pin) == LOW); // NC switch: LOW = triggered
    } else {
        reading = (digitalRead(_pin) == HIGH);
    }

    _rawState = reading;

    // Debounce
    if (reading != _lastReading) {
        _lastDebounceTime = millis();
    }
    _lastReading = reading;

    if ((millis() - _lastDebounceTime) > SWITCH_DEBOUNCE_MS) {
        _lastTriggered = _triggered;
        _triggered = reading;
    }

    // Edge detection
    _justTriggered = (_triggered && !_lastTriggered);
    _justReleased = (!_triggered && _lastTriggered);
}
