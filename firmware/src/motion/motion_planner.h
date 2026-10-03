#pragma once
// =============================================================
// Motion Planner — High-Level Motion Control
// =============================================================
// Translates Cartesian position commands into CoreXY motor moves
// and Z moves. Enforces travel limits and tracks machine position.
// =============================================================

#ifndef MOTION_PLANNER_H
#define MOTION_PLANNER_H

#include <Arduino.h>
#include "corexy_kinematics.h"
#include "stepper_controller.h"
#include "../config/config_manager.h"
#include "../utils/logger.h"

enum class PositionState : uint8_t {
    UNKNOWN,        // Never homed — position unreliable
    HOMED,          // Homed — position is known
    ESTIMATED       // Was homed but may have drifted (belt skip, power cycle)
};

class MotionPlanner {
public:
    MotionPlanner();

    void begin();

    // Configure from current settings
    void applyConfig();

    // Position commands (mm)
    bool moveToXY(float x_mm, float y_mm);
    bool moveToZ(float z_mm);
    bool moveRelativeXY(float dx_mm, float dy_mm);
    bool moveRelativeZ(float dz_mm);

    // Joystick-driven continuous movement
    // speed_x/y: -1.0 to +1.0 normalized joystick values
    void setJoystickSpeed(float speed_x, float speed_y, float speed_z);

    // Stop
    void stop();
    void emergencyStop();

    // Update — call from main loop
    bool update();

    // Position access (mm)
    float getX() const;
    float getY() const;
    float getZ() const;
    PositionState getPositionState() const { return _posState; }

    // Set position (after homing)
    void setPositionXY(float x, float y);
    void setPositionZ(float z);
    void setPositionState(PositionState state) { _posState = state; }

    // Travel limit checking
    bool isWithinLimits(float x, float y) const;
    bool isZWithinLimits(float z) const;

    // Status
    bool isMoving() const;

    // Call this from the main loop to process ongoing joystick moves
    void processJoystickMovement();

    // Direct access (for homing, calibration)
    StepperController& steppers() { return _steppers; }
    CoreXYKinematics& kinematics() { return _corexy; }

private:
    CoreXYKinematics _corexy;
    StepperController _steppers;
    PositionState _posState;

    // Joystick continuous movement
    float _joySpeedX = 0.0f;
    float _joySpeedY = 0.0f;
    float _joySpeedZ = 0.0f;
    float _lastJoyX = 0.0f;
    float _lastJoyY = 0.0f;
    float _lastJoyZ = 0.0f;
    unsigned long _lastJoyUpdate = 0;
    static constexpr unsigned long JOY_MOVE_INTERVAL_MS = 50; // 20Hz jog updates

    bool _validateAndMove(float x, float y);
    bool _isJoystickMove = false;
};

#endif // MOTION_PLANNER_H
