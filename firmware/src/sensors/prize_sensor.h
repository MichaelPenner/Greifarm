#pragma once
// =============================================================
// Prize Sensor — Break-Beam Detection with Debounce
// =============================================================

#ifndef PRIZE_SENSOR_H
#define PRIZE_SENSOR_H

#include <Arduino.h>
#include "../config/pin_config.h"
#include "../config/machine_config.h"

class PrizeSensor {
public:
    PrizeSensor();

    void begin();
    void update();

    // Prize detected (debounced, one-shot per event)
    bool prizeDetected() const { return _prizeEvent; }

    // Clear the prize event flag after handling
    void clearEvent() { _prizeEvent = false; }

    // Is beam currently broken
    bool isBeamBroken() const { return _beamBroken; }

    // Enable/disable detection (disable during non-game states)
    void setEnabled(bool enabled) { _enabled = enabled; }
    bool isEnabled() const { return _enabled; }

    // Reset cooldown (e.g., at game start)
    void resetCooldown() { _cooldownUntil = 0; }

private:
    bool _enabled;
    bool _beamBroken;
    bool _prizeEvent;
    unsigned long _beamBreakStart;
    unsigned long _cooldownUntil;
    bool _wasBlocked;
};

#endif // PRIZE_SENSOR_H
