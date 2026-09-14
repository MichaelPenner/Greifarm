#include "homing.h"

static const char* TAG = "HOMING";

HomingController::HomingController(MotionPlanner& motion, SensorManager& sensors)
    : _motion(motion), _sensors(sensors),
      _state(HomingState::IDLE), _stateStartTime(0), _fullSequence(false) {}

void HomingController::startFullHoming() {
    LOG_I(TAG, "Starting full homing sequence (Z → X → Y)");
    _fullSequence = true;
    _motion.steppers().enableMotors();
    _state = HomingState::HOMING_Z_UP;
    _stateStartTime = millis();

    // Move Z upward at homing speed
    const auto& cfg = machineConfig.settings();
    float zStepsPerSec = cfg.z_home_speed * cfg.z_steps_per_mm;
    _motion.steppers().setMaxSpeed(StepperAxis::MOTOR_Z, zStepsPerSec);

    // Command a very long move in the negative direction (toward Z-top switch)
    // The move will be interrupted when the limit switch triggers
    int32_t maxSteps = static_cast<int32_t>(cfg.z_max_mm * cfg.z_steps_per_mm * 1.5f);
    _motion.steppers().moveRelative(StepperAxis::MOTOR_Z, cfg.invert_motor_z ? maxSteps : -maxSteps);
}

void HomingController::startHomeZ() {
    LOG_I(TAG, "Homing Z axis");
    _fullSequence = false;
    _motion.steppers().enableMotors();
    _state = HomingState::HOMING_Z_UP;
    _stateStartTime = millis();

    const auto& cfg = machineConfig.settings();
    float zStepsPerSec = cfg.z_home_speed * cfg.z_steps_per_mm;
    _motion.steppers().setMaxSpeed(StepperAxis::MOTOR_Z, zStepsPerSec);
    int32_t maxSteps = static_cast<int32_t>(cfg.z_max_mm * cfg.z_steps_per_mm * 1.5f);
    _motion.steppers().moveRelative(StepperAxis::MOTOR_Z, cfg.invert_motor_z ? maxSteps : -maxSteps);
}

void HomingController::startHomeX() {
    LOG_I(TAG, "Homing X axis");
    _fullSequence = false;
    _motion.steppers().enableMotors();
    _state = HomingState::HOMING_X;
    _stateStartTime = millis();

    const auto& cfg = machineConfig.settings();
    // Move both CoreXY motors to drive X toward home (negative X direction)
    // CoreXY: X movement = Motor A and Motor B rotate same direction
    float homeSpeed = cfg.x_home_speed * cfg.steps_per_mm;
    _motion.steppers().setMaxSpeed(StepperAxis::MOTOR_A, homeSpeed);
    _motion.steppers().setMaxSpeed(StepperAxis::MOTOR_B, homeSpeed);

    int32_t maxSteps = static_cast<int32_t>(cfg.x_max_mm * cfg.steps_per_mm * 1.5f);
    MotorSteps ms = _motion.kinematics().cartesianToMotor(-cfg.x_max_mm * 1.5f, 0);
    _motion.steppers().moveSynchronized(
        _motion.steppers().getPosition(StepperAxis::MOTOR_A) + ms.a,
        _motion.steppers().getPosition(StepperAxis::MOTOR_B) + ms.b
    );
}

void HomingController::startHomeY() {
    LOG_I(TAG, "Homing Y axis");
    _fullSequence = false;
    _motion.steppers().enableMotors();
    _state = HomingState::HOMING_Y;
    _stateStartTime = millis();

    const auto& cfg = machineConfig.settings();
    float homeSpeed = cfg.y_home_speed * cfg.steps_per_mm;
    _motion.steppers().setMaxSpeed(StepperAxis::MOTOR_A, homeSpeed);
    _motion.steppers().setMaxSpeed(StepperAxis::MOTOR_B, homeSpeed);

    MotorSteps ms = _motion.kinematics().cartesianToMotor(0, -cfg.y_max_mm * 1.5f);
    _motion.steppers().moveSynchronized(
        _motion.steppers().getPosition(StepperAxis::MOTOR_A) + ms.a,
        _motion.steppers().getPosition(StepperAxis::MOTOR_B) + ms.b
    );
}

void HomingController::cancel() {
    LOG_W(TAG, "Homing cancelled");
    _motion.steppers().haltAll();
    _state = HomingState::IDLE;
}

bool HomingController::update() {
    if (_state == HomingState::IDLE || _state == HomingState::COMPLETE ||
        _state == HomingState::FAILED) {
        return false;
    }

    if (_isTimedOut()) {
        _fail("Homing timed out — check limit switches");
        return false;
    }

    const auto& cfg = machineConfig.settings();

    switch (_state) {
        case HomingState::HOMING_Z_UP:
            if (_sensors.isZTopTriggered()) {
                _motion.steppers().haltAll();
                LOG_I(TAG, "Z-top switch triggered, backing off");
                _state = HomingState::HOMING_Z_BACKOFF;
                _stateStartTime = millis();

                // Back off from switch
                int32_t backoff = static_cast<int32_t>(cfg.z_home_backoff * cfg.z_steps_per_mm);
                _motion.steppers().moveRelative(StepperAxis::MOTOR_Z,
                    cfg.invert_motor_z ? -backoff : backoff);
            }
            break;

        case HomingState::HOMING_Z_BACKOFF:
            if (!_motion.steppers().isMoving(StepperAxis::MOTOR_Z)) {
                // Z is homed — set position to 0 (top)
                _motion.steppers().setPosition(StepperAxis::MOTOR_Z, 0);
                LOG_I(TAG, "Z homed successfully");

                // Restore Z speed
                _motion.steppers().setMaxSpeed(StepperAxis::MOTOR_Z,
                    cfg.z_max_speed * cfg.z_steps_per_mm);

                if (_fullSequence) {
                    // Continue to X homing
                    _state = HomingState::HOMING_X;
                    _stateStartTime = millis();
                    startHomeX();
                    _fullSequence = true; // Restore flag
                } else {
                    _state = HomingState::COMPLETE;
                    _motion.setPositionState(PositionState::HOMED);
                }
            }
            break;

        case HomingState::HOMING_X:
            if (_sensors.isXHomeTriggered()) {
                _motion.steppers().haltAll();
                LOG_I(TAG, "X-home switch triggered, backing off");
                _state = HomingState::HOMING_X_BACKOFF;
                _stateStartTime = millis();

                // Back off from switch (positive X direction)
                MotorSteps backoff = _motion.kinematics().cartesianToMotor(cfg.x_home_backoff, 0);
                _motion.steppers().moveSynchronized(
                    _motion.steppers().getPosition(StepperAxis::MOTOR_A) + backoff.a,
                    _motion.steppers().getPosition(StepperAxis::MOTOR_B) + backoff.b
                );
            }
            break;

        case HomingState::HOMING_X_BACKOFF:
            if (!_motion.steppers().isMoving(StepperAxis::MOTOR_A) &&
                !_motion.steppers().isMoving(StepperAxis::MOTOR_B)) {
                LOG_I(TAG, "X homed successfully");

                // Set X=0 (keep current Y motor position relative)
                // After X home, we know X position but Y is still unknown
                _motion.setPositionXY(0, _motion.getY());

                // Restore speeds
                float motorSpeed = _motion.kinematics().maxMotorSpeedForCartesian(
                    cfg.x_max_speed) * cfg.steps_per_mm;
                _motion.steppers().setMaxSpeed(StepperAxis::MOTOR_A, motorSpeed);
                _motion.steppers().setMaxSpeed(StepperAxis::MOTOR_B, motorSpeed);

                if (_fullSequence) {
                    _state = HomingState::HOMING_Y;
                    _stateStartTime = millis();
                    startHomeY();
                    _fullSequence = true;
                } else {
                    _state = HomingState::COMPLETE;
                }
            }
            break;

        case HomingState::HOMING_Y:
            if (_sensors.isYHomeTriggered()) {
                _motion.steppers().haltAll();
                LOG_I(TAG, "Y-home switch triggered, backing off");
                _state = HomingState::HOMING_Y_BACKOFF;
                _stateStartTime = millis();

                MotorSteps backoff = _motion.kinematics().cartesianToMotor(0, cfg.y_home_backoff);
                _motion.steppers().moveSynchronized(
                    _motion.steppers().getPosition(StepperAxis::MOTOR_A) + backoff.a,
                    _motion.steppers().getPosition(StepperAxis::MOTOR_B) + backoff.b
                );
            }
            break;

        case HomingState::HOMING_Y_BACKOFF:
            if (!_motion.steppers().isMoving(StepperAxis::MOTOR_A) &&
                !_motion.steppers().isMoving(StepperAxis::MOTOR_B)) {
                LOG_I(TAG, "Y homed successfully");

                // Now both X and Y are known
                _motion.setPositionXY(0, 0);
                _motion.setPositionZ(0);
                _motion.setPositionState(PositionState::HOMED);

                // Restore full speeds
                float motorSpeed = _motion.kinematics().maxMotorSpeedForCartesian(
                    max(cfg.x_max_speed, cfg.y_max_speed)) * cfg.steps_per_mm;
                _motion.steppers().setMaxSpeed(StepperAxis::MOTOR_A, motorSpeed);
                _motion.steppers().setMaxSpeed(StepperAxis::MOTOR_B, motorSpeed);

                if (_fullSequence) {
                    // Move to park position
                    _state = HomingState::MOVING_TO_PARK;
                    _stateStartTime = millis();
                    _motion.moveToXY(cfg.park_x, cfg.park_y);
                } else {
                    _state = HomingState::COMPLETE;
                }
            }
            break;

        case HomingState::MOVING_TO_PARK:
            if (!_motion.isMoving()) {
                LOG_I(TAG, "Full homing complete — at park position");
                _state = HomingState::COMPLETE;
            }
            break;

        default:
            break;
    }

    // Keep steppers running during homing
    _motion.steppers().update();
    return true;
}

bool HomingController::_isTimedOut() const {
    return (millis() - _stateStartTime) > HOMING_TIMEOUT_MS;
}

void HomingController::_fail(const char* reason) {
    LOG_E(TAG, "Homing FAILED: %s", reason);
    _motion.steppers().haltAll();
    _state = HomingState::FAILED;
}
