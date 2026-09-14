#pragma once
// =============================================================
// Claw Controller — Servo-Based Claw Grip
// =============================================================

#ifndef CLAW_CONTROLLER_H
#define CLAW_CONTROLLER_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include "../config/pin_config.h"
#include "../config/config_manager.h"
#include "../utils/logger.h"
#include "../utils/timer.h"

enum class ClawState : uint8_t {
    UNKNOWN,
    OPEN,
    CLOSED,
    PARTIAL,
    MOVING
};

class ClawController {
public:
    ClawController();

    void begin();

    // Claw commands
    void open();
    void close();
    void closeWithStrength(int strengthPercent); // 0–100
    void partial();
    void setAngle(int angle);

    // Update — handles non-blocking servo movement
    void update();

    // State
    ClawState getState() const { return _state; }
    int getCurrentAngle() const { return _currentAngle; }
    int getTargetAngle() const { return _targetAngle; }
    bool isMoving() const { return _state == ClawState::MOVING; }

    // Check if grip delay has completed (after close)
    bool isGripComplete() const { return _gripComplete; }
    void clearGripComplete() { _gripComplete = false; }

    // Detach servo (to save power/stop jitter when not needed)
    void detach();
    void attach();
    bool isAttached() const { return _attached; }

private:
    Servo _servo;
    ClawState _state;
    int _currentAngle;
    int _targetAngle;
    bool _attached;
    bool _gripComplete;
    OneShotTimer _gripTimer;
    OneShotTimer _moveTimer;
};

#endif // CLAW_CONTROLLER_H
