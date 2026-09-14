#pragma once
// =============================================================
// Limit Switch — Debounced Digital Input
// =============================================================

#ifndef LIMIT_SWITCH_H
#define LIMIT_SWITCH_H

#include <Arduino.h>
#include "../config/machine_config.h"

class LimitSwitch {
public:
    LimitSwitch(uint8_t pin, bool activeLow = SWITCH_ACTIVE_LOW);

    void begin();

    // Update debounce state — call frequently
    void update();

    // Returns true when switch is triggered (debounced)
    bool isTriggered() const { return _triggered; }

    // Edge detection
    bool justTriggered() const { return _justTriggered; }
    bool justReleased() const { return _justReleased; }

    // Raw (un-debounced) reading
    bool rawState() const { return _rawState; }

    uint8_t getPin() const { return _pin; }

private:
    uint8_t _pin;
    bool _activeLow;
    bool _triggered;
    bool _lastTriggered;
    bool _justTriggered;
    bool _justReleased;
    bool _rawState;
    bool _lastReading;
    unsigned long _lastDebounceTime;
};

#endif // LIMIT_SWITCH_H
