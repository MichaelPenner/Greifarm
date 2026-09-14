#pragma once
// =============================================================
// Homing — Axis Homing Sequences
// =============================================================

#ifndef HOMING_H
#define HOMING_H

#include <Arduino.h>
#include "motion_planner.h"
#include "../sensors/sensor_manager.h"
#include "../utils/logger.h"

enum class HomingState : uint8_t {
    IDLE,
    HOMING_Z_UP,       // Move Z up until Z-top switch
    HOMING_Z_BACKOFF,  // Back off Z-top switch
    HOMING_X,          // Move X until X-home switch
    HOMING_X_BACKOFF,  // Back off X-home switch
    HOMING_Y,          // Move Y until Y-home switch
    HOMING_Y_BACKOFF,  // Back off Y-home switch
    MOVING_TO_PARK,    // Move to park position
    COMPLETE,
    FAILED
};

class HomingController {
public:
    HomingController(MotionPlanner& motion, SensorManager& sensors);

    // Start full homing sequence (Z first, then X, then Y)
    void startFullHoming();

    // Home individual axes
    void startHomeZ();
    void startHomeX();
    void startHomeY();

    // Cancel homing
    void cancel();

    // Update — call from main loop. Returns true while homing is active.
    bool update();

    // Status
    HomingState getState() const { return _state; }
    bool isHoming() const { return _state != HomingState::IDLE &&
                                   _state != HomingState::COMPLETE &&
                                   _state != HomingState::FAILED; }
    bool isComplete() const { return _state == HomingState::COMPLETE; }
    bool hasFailed() const { return _state == HomingState::FAILED; }

    // Reset state (after reading result)
    void reset() { _state = HomingState::IDLE; }

private:
    MotionPlanner& _motion;
    SensorManager& _sensors;
    HomingState _state;
    unsigned long _stateStartTime;
    bool _fullSequence;  // true if running full Z→X→Y sequence

    static constexpr unsigned long HOMING_TIMEOUT_MS = 30000; // 30 sec max per axis

    void _startMoveToward(float speed_mm_s, int8_t dir_x, int8_t dir_y, int8_t dir_z);
    bool _isTimedOut() const;
    void _fail(const char* reason);
};

#endif // HOMING_H
