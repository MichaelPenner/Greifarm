#include "motion_planner.h"

static const char* TAG = "MOTION";

MotionPlanner::MotionPlanner()
    : _posState(PositionState::UNKNOWN),
      _joySpeedX(0), _joySpeedY(0), _joySpeedZ(0),
      _lastJoyUpdate(0) {}

void MotionPlanner::begin() {
    _steppers.begin();
    applyConfig();
    LOG_I(TAG, "Motion planner initialized");
}

void MotionPlanner::applyConfig() {
    const auto& cfg = machineConfig.settings();

    // CoreXY setup
    _corexy.setStepsPerMm(cfg.steps_per_mm);
    _corexy.setInvertA(cfg.invert_motor_a);
    _corexy.setInvertB(cfg.invert_motor_b);

    // Motor speeds (convert mm/s to steps/s)
    // For CoreXY motors, use the higher of X/Y max speed × sqrt(2)
    float maxXYSpeed = max(cfg.x_max_speed, cfg.y_max_speed);
    float motorSpeed = _corexy.maxMotorSpeedForCartesian(maxXYSpeed) * cfg.steps_per_mm;
    float motorAccel = _corexy.maxMotorSpeedForCartesian(
        max(cfg.x_acceleration, cfg.y_acceleration)) * cfg.steps_per_mm;

    _steppers.setMaxSpeed(StepperAxis::MOTOR_A, motorSpeed);
    _steppers.setMaxSpeed(StepperAxis::MOTOR_B, motorSpeed);
    _steppers.setAcceleration(StepperAxis::MOTOR_A, motorAccel);
    _steppers.setAcceleration(StepperAxis::MOTOR_B, motorAccel);

    // Z axis
    _steppers.setMaxSpeed(StepperAxis::MOTOR_Z, cfg.z_max_speed * cfg.z_steps_per_mm);
    _steppers.setAcceleration(StepperAxis::MOTOR_Z, cfg.z_acceleration * cfg.z_steps_per_mm);

    LOG_I(TAG, "Config applied: spm=%.1f, x_max=%.0f, y_max=%.0f, z_max=%.0f",
          cfg.steps_per_mm, cfg.x_max_mm, cfg.y_max_mm, cfg.z_max_mm);
}

bool MotionPlanner::moveToXY(float x_mm, float y_mm) {
    if (_posState == PositionState::UNKNOWN) {
        LOG_E(TAG, "Cannot move — position unknown, home first");
        return false;
    }

    if (!isWithinLimits(x_mm, y_mm)) {
        LOG_W(TAG, "Target out of bounds: X=%.1f Y=%.1f", x_mm, y_mm);
        return false;
    }

    return _validateAndMove(x_mm, y_mm);
}

bool MotionPlanner::moveToZ(float z_mm) {
    if (_posState == PositionState::UNKNOWN) {
        LOG_E(TAG, "Cannot move Z — position unknown, home first");
        return false;
    }

    if (!isZWithinLimits(z_mm)) {
        LOG_W(TAG, "Z target out of bounds: %.1f", z_mm);
        return false;
    }

    const auto& cfg = machineConfig.settings();
    int32_t targetSteps = static_cast<int32_t>(z_mm * cfg.z_steps_per_mm);

    if (cfg.invert_motor_z) targetSteps = -targetSteps;

    _steppers.moveTo(StepperAxis::MOTOR_Z, targetSteps);
    return true;
}

bool MotionPlanner::moveRelativeXY(float dx_mm, float dy_mm) {
    float newX = getX() + dx_mm;
    float newY = getY() + dy_mm;
    return moveToXY(newX, newY);
}

bool MotionPlanner::moveRelativeZ(float dz_mm) {
    float newZ = getZ() + dz_mm;
    return moveToZ(newZ);
}

void MotionPlanner::setJoystickSpeed(float speed_x, float speed_y, float speed_z) {
    // Clamp inputs to safe range to prevent oversized values from buggy clients
    _joySpeedX = constrain(speed_x, -1.0f, 1.0f);
    _joySpeedY = constrain(speed_y, -1.0f, 1.0f);
    _joySpeedZ = constrain(speed_z, -1.0f, 1.0f);
}

void MotionPlanner::stop() {
    _joySpeedX = 0;
    _joySpeedY = 0;
    _joySpeedZ = 0;
    _steppers.stopAll();
}

void MotionPlanner::emergencyStop() {
    _joySpeedX = 0;
    _joySpeedY = 0;
    _joySpeedZ = 0;
    _steppers.haltAll();
    _steppers.disableMotors();
    _posState = PositionState::ESTIMATED; // Position may be off after emergency halt
    LOG_E(TAG, "EMERGENCY STOP — motors halted and disabled");
}

bool MotionPlanner::update() {
    // Only stepper pulsing happens here (runs in 20kHz timer task)
    return _steppers.update();
}

float MotionPlanner::getX() const {
    int32_t a = _steppers.getPosition(StepperAxis::MOTOR_A);
    int32_t b = _steppers.getPosition(StepperAxis::MOTOR_B);
    CartesianPosition pos = _corexy.motorToCartesian(a, b);
    return pos.x;
}

float MotionPlanner::getY() const {
    int32_t a = _steppers.getPosition(StepperAxis::MOTOR_A);
    int32_t b = _steppers.getPosition(StepperAxis::MOTOR_B);
    CartesianPosition pos = _corexy.motorToCartesian(a, b);
    return pos.y;
}

float MotionPlanner::getZ() const {
    const auto& cfg = machineConfig.settings();
    int32_t steps = _steppers.getPosition(StepperAxis::MOTOR_Z);
    if (cfg.invert_motor_z) steps = -steps;
    return static_cast<float>(steps) / cfg.z_steps_per_mm;
}

void MotionPlanner::setPositionXY(float x, float y) {
    MotorSteps ms = _corexy.cartesianToMotor(x, y);
    // Set absolute position (not relative from current)
    _steppers.setPosition(StepperAxis::MOTOR_A, ms.a);
    _steppers.setPosition(StepperAxis::MOTOR_B, ms.b);
}

void MotionPlanner::setPositionZ(float z) {
    const auto& cfg = machineConfig.settings();
    int32_t steps = static_cast<int32_t>(z * cfg.z_steps_per_mm);
    if (cfg.invert_motor_z) steps = -steps;
    _steppers.setPosition(StepperAxis::MOTOR_Z, steps);
}

bool MotionPlanner::isWithinLimits(float x, float y) const {
    const auto& cfg = machineConfig.settings();
    return (x >= 0 && x <= cfg.x_max_mm &&
            y >= 0 && y <= cfg.y_max_mm);
}

bool MotionPlanner::isZWithinLimits(float z) const {
    const auto& cfg = machineConfig.settings();
    return (z >= 0 && z <= cfg.z_max_mm);
}

bool MotionPlanner::isMoving() const {
    return _steppers.isAnyMoving();
}

bool MotionPlanner::_validateAndMove(float x, float y) {
    MotorSteps target = _corexy.cartesianToMotor(x, y);
    _steppers.moveSynchronized(target.a, target.b);
    return true;
}

void MotionPlanner::processJoystickMovement() {
    unsigned long now = millis();
    if (now - _lastJoyUpdate < JOY_MOVE_INTERVAL_MS) return;
    _lastJoyUpdate = now;

    const auto& cfg = machineConfig.settings();

    bool hasInput = (abs(_joySpeedX) > 0.01f || abs(_joySpeedY) > 0.01f || abs(_joySpeedZ) > 0.01f);

    if (!hasInput) {
        if (_isJoystickMove) {
            _steppers.stopAll();
            _isJoystickMove = false;
        }
        return;
    }

    // If the steppers reached their target but the joystick is still held,
    // we must clear the flag so it can plan a new move.
    if (_isJoystickMove && !_steppers.isAnyMoving()) {
        _isJoystickMove = false;
    }

    // Ignore if a non-joystick move is currently running
    if (!_isJoystickMove && 
        (_steppers.isMoving(StepperAxis::MOTOR_A) ||
         _steppers.isMoving(StepperAxis::MOTOR_B) ||
         _steppers.isMoving(StepperAxis::MOTOR_Z))) {
        return;
    }

    bool inputChanged = (_joySpeedX != _lastJoyX) || (_joySpeedY != _lastJoyY) || (_joySpeedZ != _lastJoyZ);
    _lastJoyX = _joySpeedX;
    _lastJoyY = _joySpeedY;
    _lastJoyZ = _joySpeedZ;

    // If input changed, stop current move and re-plan immediately
    if (inputChanged && _isJoystickMove) {
        _steppers.stopAll();
        _isJoystickMove = false;
    }

    // Start a new joystick move only if we aren't already running one
    if (!_isJoystickMove) {
        // Use large relative moves in the joystick direction, clamped to limits.
        // The move will be re-planned when joystick input changes or is released.
        float moveDistMm = 500.0f; // Large enough to keep moving until released

        float targetX = getX() + _joySpeedX * moveDistMm;
        float targetY = getY() + _joySpeedY * moveDistMm;
        float targetZ = getZ() + _joySpeedZ * moveDistMm;

        // Clamp to software limits — critical safety, but only if homed!
        // [TEMPORARY OVERRIDE] Disable software limits entirely so the user can manually test full travel.
        // if (_posState != PositionState::UNKNOWN) {
        //     targetX = constrain(targetX, 0.0f, cfg.x_max_mm);
        //     targetY = constrain(targetY, 0.0f, cfg.y_max_mm);
        //     targetZ = constrain(targetZ, 0.0f, cfg.z_max_mm);
        // }

        if (abs(_joySpeedX) > 0.01f || abs(_joySpeedY) > 0.01f) {
            _validateAndMove(targetX, targetY);
        }
        
        if (abs(_joySpeedZ) > 0.01f) {
            int32_t targetZSteps = static_cast<int32_t>(targetZ * cfg.z_steps_per_mm);
            if (cfg.invert_motor_z) targetZSteps = -targetZSteps;
            _steppers.moveTo(StepperAxis::MOTOR_Z, targetZSteps);
        }

        _isJoystickMove = true;
    }
}

