#pragma once
// =============================================================
// Safety Controller — Emergency Stop + Watchdog + Interlocks
// =============================================================

#ifndef SAFETY_CONTROLLER_H
#define SAFETY_CONTROLLER_H

#include <Arduino.h>
#include "../sensors/sensor_manager.h"
#include "../motion/motion_planner.h"
#include "../claw/claw_controller.h"
#include "../utils/logger.h"
#include "../utils/timer.h"
#include "error_codes.h"

enum class SafetyState : uint8_t {
    OK,
    WARNING,
    ESTOP_ACTIVE,
    ERROR
};

class SafetyController {
public:
    SafetyController(MotionPlanner& motion, SensorManager& sensors, ClawController& claw);

    void begin();

    // Must be called every loop iteration — BEFORE motion update
    void update();

    // Safety state
    SafetyState getState() const { return _safetyState; }
    bool isOk() const { return _safetyState == SafetyState::OK; }
    bool isEStopActive() const { return _safetyState == SafetyState::ESTOP_ACTIVE; }

    // Error access
    ErrorCode getLastError() const { return _lastError; }
    const char* getLastErrorMessage() const { return errorMessage(_lastError); }
    void clearError();

    // Movement permission
    bool isMovementAllowed() const;

    // Communication heartbeat — call when message received from backend
    void heartbeatReceived();
    bool isCommAlive() const { return _commAlive; }

    // Reset after E-stop (requires E-stop button to be released)
    bool resetEStop();

    // Motion watchdog — call when starting a planned move
    void startMoveWatchdog();
    void cancelMoveWatchdog();

private:
    MotionPlanner& _motion;
    SensorManager& _sensors;
    ClawController& _claw;

    SafetyState _safetyState;
    ErrorCode _lastError;

    // Communication heartbeat
    unsigned long _lastHeartbeat;
    bool _commAlive;

    // Motion watchdog
    OneShotTimer _moveWatchdog;
    bool _moveWatchdogActive;

    // Motor idle timer
    unsigned long _lastMotionTime;
    bool _idleHandled;

    void _triggerEStop(ErrorCode reason);
    void _checkLimitSwitches();
    void _checkCommunication();
    void _checkMoveWatchdog();
    void _checkMotorIdle();
    void _setError(ErrorCode code);
};

#endif // SAFETY_CONTROLLER_H
