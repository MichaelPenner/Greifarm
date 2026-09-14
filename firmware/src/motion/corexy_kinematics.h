#pragma once
// =============================================================
// CoreXY Kinematics — Cartesian ↔ Motor Transformation
// =============================================================
// CoreXY uses two motors (A, B) to drive both X and Y axes
// through a crossed-belt system.
//
// Forward transform (Cartesian → Motors):
//   motor_a_steps = delta_x_steps + delta_y_steps
//   motor_b_steps = delta_x_steps - delta_y_steps
//
// Inverse transform (Motors → Cartesian):
//   delta_x_steps = 0.5 * (motor_a_steps + motor_b_steps)
//   delta_y_steps = 0.5 * (motor_a_steps - motor_b_steps)
//
// Motor behavior:
//   Both same direction    → Y movement
//   Both opposite direction → X movement
//   Only one motor         → 45° diagonal
// =============================================================

#ifndef COREXY_KINEMATICS_H
#define COREXY_KINEMATICS_H

#include <cmath>
#include <cstdint>

struct CartesianPosition {
    float x;    // mm
    float y;    // mm
};

struct MotorSteps {
    int32_t a;  // Motor A steps
    int32_t b;  // Motor B steps
};

class CoreXYKinematics {
public:
    CoreXYKinematics();

    // Configuration
    void setStepsPerMm(float stepsPerMm);
    void setInvertA(bool invert);
    void setInvertB(bool invert);

    // Forward kinematics: Cartesian delta (mm) → Motor steps delta
    MotorSteps cartesianToMotor(float dx_mm, float dy_mm) const;

    // Inverse kinematics: Motor steps delta → Cartesian delta (mm)
    CartesianPosition motorToCartesian(int32_t da_steps, int32_t db_steps) const;

    // Convert mm to steps for a single axis
    int32_t mmToSteps(float mm) const;

    // Convert steps to mm for a single axis
    float stepsToMm(int32_t steps) const;

    // Calculate the maximum motor speed for a given Cartesian speed
    // In CoreXY, diagonal moves require sqrt(2)× the motor speed
    float maxMotorSpeedForCartesian(float cartesian_speed_mm_s) const;

    float getStepsPerMm() const { return _stepsPerMm; }

private:
    float _stepsPerMm;
    int8_t _dirA;   // +1 or -1
    int8_t _dirB;   // +1 or -1
};

#endif // COREXY_KINEMATICS_H
