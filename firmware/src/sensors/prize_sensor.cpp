#include "prize_sensor.h"
#include "../utils/logger.h"

static const char* TAG = "PRIZE";

PrizeSensor::PrizeSensor()
    : _enabled(false), _beamBroken(false), _prizeEvent(false),
      _beamBreakStart(0), _cooldownUntil(0), _wasBlocked(false) {}

void PrizeSensor::begin() {
    pinMode(PIN_PRIZE_SENSOR, INPUT_PULLUP);
    LOG_I(TAG, "Prize sensor initialized on pin %d", PIN_PRIZE_SENSOR);
}

void PrizeSensor::update() {
    if (!_enabled) {
        _beamBroken = false;
        _wasBlocked = false;
        return;
    }

    // Active LOW: beam broken = pin LOW
    bool currentlyBlocked = (digitalRead(PIN_PRIZE_SENSOR) == LOW);
    _beamBroken = currentlyBlocked;

    // Check cooldown
    if (millis() < _cooldownUntil) {
        _wasBlocked = currentlyBlocked;
        return;
    }

    if (currentlyBlocked && !_wasBlocked) {
        // Beam just broken — start timing
        _beamBreakStart = millis();
    } else if (currentlyBlocked && _wasBlocked) {
        // Beam still broken — check minimum duration
        if ((millis() - _beamBreakStart) >= PRIZE_MIN_BLOCK_MS && !_prizeEvent) {
            // Valid prize detection!
            _prizeEvent = true;
            _cooldownUntil = millis() + PRIZE_COOLDOWN_MS;
            LOG_I(TAG, "🎉 PRIZE DETECTED!");
        }
    } else if (!currentlyBlocked && _wasBlocked) {
        // Beam restored
        if ((millis() - _beamBreakStart) < PRIZE_MIN_BLOCK_MS) {
            // Too short — ignore (noise/flicker)
            LOG_D(TAG, "Beam break too short, ignored (noise)");
        }
    }

    _wasBlocked = currentlyBlocked;
}
