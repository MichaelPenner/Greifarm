#pragma once
// =============================================================
// Stepper Controller — Low-Level Step Pulse Generation
// =============================================================
// Generates step pulses with acceleration/deceleration for
// individual stepper motors via A4988 drivers.
//
// Uses timer-based step generation with trapezoidal velocity
// profile (constant acceleration).
// =============================================================

#ifndef STEPPER_CONTROLLER_H
#define STEPPER_CONTROLLER_H

#include <Arduino.h>
#include <cstdint>
#include "../config/pin_config.h"
#include "../config/machine_config.h"

// Stepper axis identifier
enum class StepperAxis : uint8_t {
    MOTOR_A = 0,
    MOTOR_B = 1,
    MOTOR_Z = 2
};

// Movement state for a single stepper
struct StepperState {
    // Pin assignments
    uint8_t stepPin;
    uint8_t dirPin;

    // Position tracking
    volatile int32_t currentSteps;  // Current absolute position in steps
    int32_t targetSteps;            // Target position in steps

    // Motion parameters (in steps)
    float maxSpeed;         // steps/sec
    float acceleration;     // steps/sec²
    float currentSpeed;     // Current instantaneous speed (steps/sec)

    // Move state
    volatile int32_t stepsRemaining;
    int8_t direction;       // +1 or -1
    bool moving;

    // Acceleration profile
    int32_t accelSteps;     // Steps to accelerate
    int32_t decelStart;     // Step count where deceleration begins
    int32_t totalSteps;     // Total steps in current move
    float stepInterval;     // Current interval between steps (µs)
    float minInterval;      // Minimum interval (max speed)
    unsigned long lastStepTime; // Timestamp of last step (µs)
};

class StepperController {
public:
    StepperController();

    // Initialize GPIO pins and state
    void begin();

    // Configure axis parameters
    void setMaxSpeed(StepperAxis axis, float stepsPerSec);
    void setAcceleration(StepperAxis axis, float stepsPerSecSq);

    // Enable/disable all motor drivers (shared ENABLE pin)
    void enableMotors();
    void disableMotors();
    bool areMotorsEnabled() const { return _motorsEnabled; }

    // Absolute position move
    void moveTo(StepperAxis axis, int32_t targetSteps);

    // Relative move
    void moveRelative(StepperAxis axis, int32_t deltaSteps);

    // Move two axes simultaneously (for CoreXY)
    void moveSynchronized(int32_t targetA, int32_t targetB);

    // Must be called from main loop as fast as possible
    // Returns true if any axis is still moving
    bool update();

    // Stop immediately (deceleration)
    void stopAll();

    // Stop immediately (no deceleration — emergency only)
    void haltAll();

    // Position access
    int32_t getPosition(StepperAxis axis) const;
    void setPosition(StepperAxis axis, int32_t steps);
    void resetPosition(StepperAxis axis);

    // Status
    bool isMoving(StepperAxis axis) const;
    bool isAnyMoving() const;

private:
    StepperState _axes[3];
    bool _motorsEnabled;

    void _initAxis(StepperAxis axis, uint8_t stepPin, uint8_t dirPin);
    void _planMove(StepperState& s);
    bool _stepAxis(StepperState& s);
    void _step(StepperState& s);
};

#endif // STEPPER_CONTROLLER_H
