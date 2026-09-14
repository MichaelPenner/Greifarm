#include "corexy_kinematics.h"

CoreXYKinematics::CoreXYKinematics()
    : _stepsPerMm(80.0f), _dirA(1), _dirB(1) {}

void CoreXYKinematics::setStepsPerMm(float stepsPerMm) {
    _stepsPerMm = stepsPerMm;
}

void CoreXYKinematics::setInvertA(bool invert) {
    _dirA = invert ? -1 : 1;
}

void CoreXYKinematics::setInvertB(bool invert) {
    _dirB = invert ? -1 : 1;
}

MotorSteps CoreXYKinematics::cartesianToMotor(float dx_mm, float dy_mm) const {
    // CoreXY forward transform:
    //   Δa = Δx + Δy  (in mm)
    //   Δb = Δx - Δy  (in mm)
    float da_mm = dx_mm + dy_mm;
    float db_mm = dx_mm - dy_mm;

    MotorSteps result;
    result.a = static_cast<int32_t>(roundf(da_mm * _stepsPerMm)) * _dirA;
    result.b = static_cast<int32_t>(roundf(db_mm * _stepsPerMm)) * _dirB;

    return result;
}

CartesianPosition CoreXYKinematics::motorToCartesian(int32_t da_steps, int32_t db_steps) const {
    // Apply direction correction
    float da = static_cast<float>(da_steps * _dirA) / _stepsPerMm;
    float db = static_cast<float>(db_steps * _dirB) / _stepsPerMm;

    // CoreXY inverse transform:
    //   Δx = 0.5 × (Δa + Δb)
    //   Δy = 0.5 × (Δa - Δb)
    CartesianPosition result;
    result.x = 0.5f * (da + db);
    result.y = 0.5f * (da - db);

    return result;
}

int32_t CoreXYKinematics::mmToSteps(float mm) const {
    return static_cast<int32_t>(roundf(mm * _stepsPerMm));
}

float CoreXYKinematics::stepsToMm(int32_t steps) const {
    return static_cast<float>(steps) / _stepsPerMm;
}

float CoreXYKinematics::maxMotorSpeedForCartesian(float cartesian_speed_mm_s) const {
    // In CoreXY, a pure X or pure Y move requires both motors at the same speed.
    // A diagonal move (X+Y simultaneously) at speed S requires each motor at S.
    // A pure X or Y move at speed S requires each motor at S (not sqrt(2)*S),
    // because Δa = Δx + Δy and for pure X: Δa = Δx, Δb = Δx.
    // The worst case is a 45° diagonal where one motor does all the work.
    // For safety, we scale by sqrt(2) to ensure the motors can handle any direction.
    return cartesian_speed_mm_s * 1.414f;
}
