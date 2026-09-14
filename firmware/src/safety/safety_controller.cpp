#include "safety_controller.h"

static const char* TAG = "SAFETY";

SafetyController::SafetyController(MotionPlanner& motion, SensorManager& sensors,
                                     ClawController& claw)
    : _motion(motion), _sensors(sensors), _claw(claw),
      _safetyState(SafetyState::OK), _lastError(ErrorCode::NONE),
      _lastHeartbeat(0), _commAlive(false),
      _moveWatchdogActive(false), _lastMotionTime(0) {}

void SafetyController::begin() {
    _lastHeartbeat = millis();
    _lastMotionTime = millis();
    LOG_I(TAG, "Safety controller initialized");
}

void SafetyController::update() {
    // 1. Check E-stop button FIRST (highest priority)
    if (_sensors.eStopJustTriggered()) {
        _triggerEStop(ErrorCode::EMERGENCY_STOP);
        return;
    }

    // If E-stop is active, don't do other checks
    if (_safetyState == SafetyState::ESTOP_ACTIVE) {
        return;
    }

    // 2. Check limit switches during movement
    _checkLimitSwitches();

    // 3. Check communication heartbeat
    _checkCommunication();

    // 4. Check motion watchdog
    _checkMoveWatchdog();

    // 5. Check motor idle timeout
    _checkMotorIdle();
}

void SafetyController::_triggerEStop(ErrorCode reason) {
    if (_safetyState == SafetyState::ESTOP_ACTIVE) return; // Already in E-stop

    LOG_E(TAG, "🛑 E-STOP TRIGGERED: %s", errorMessage(reason));

    _motion.emergencyStop();
    _claw.open(); // Release anything held
    _safetyState = SafetyState::ESTOP_ACTIVE;
    _lastError = reason;
}

void SafetyController::_checkLimitSwitches() {
    // During normal movement (not homing), a limit switch trigger means
    // we've hit a mechanical boundary — stop immediately
    if (_motion.isMoving()) {
        // Z bottom limit — stop Z motor
        if (_sensors.isZBottomTriggered()) {
            LOG_W(TAG, "Z-bottom limit triggered during move — stopping Z");
            _motion.steppers().haltAll();
            _setError(ErrorCode::LIMIT_TRIGGERED);
        }

        // Z top limit — stop Z motor (during non-homing moves)
        if (_sensors.isZTopTriggered()) {
            LOG_W(TAG, "Z-top limit triggered during move");
            _motion.steppers().haltAll();
            _setError(ErrorCode::LIMIT_TRIGGERED);
        }
    }

    // Track last motion time
    if (_motion.isMoving()) {
        _lastMotionTime = millis();
    }
}

void SafetyController::_checkCommunication() {
    unsigned long timeSinceHeartbeat = millis() - _lastHeartbeat;

    if (timeSinceHeartbeat > HEARTBEAT_TIMEOUT_MS) {
        if (_commAlive) {
            LOG_W(TAG, "Communication heartbeat lost (%lu ms)", timeSinceHeartbeat);
            _commAlive = false;
            // Don't trigger E-stop for comm loss — just stop joystick
            _motion.setJoystickSpeed(0, 0, 0);
        }
    } else {
        if (!_commAlive) {
            LOG_I(TAG, "Communication restored");
        }
        _commAlive = true;
    }
}

void SafetyController::_checkMoveWatchdog() {
    if (_moveWatchdogActive && _moveWatchdog.hasExpired()) {
        LOG_E(TAG, "Motion watchdog expired — aborting move");
        _motion.stop();
        _setError(ErrorCode::WATCHDOG_TIMEOUT);
        _moveWatchdogActive = false;
    }
}

void SafetyController::_checkMotorIdle() {
    const auto& cfg = machineConfig.settings();
    unsigned long idle = millis() - _lastMotionTime;

    if (idle > MOTOR_IDLE_TIMEOUT_MS && _motion.steppers().areMotorsEnabled()) {
        if (!cfg.z_always_energized) {
            LOG_I(TAG, "Motor idle timeout — disabling motors");
            _motion.steppers().disableMotors();
        } else {
            // Only disable XY motors, keep Z energized
            // Note: CNC Shield has a shared enable pin, so we can't
            // selectively disable. In a production build, this would
            // require individual enable wiring.
            // For now, keep all motors energized if Z needs to stay on.
            LOG_D(TAG, "Motors idle but Z-hold enabled — keeping energized");
        }
    }
}

bool SafetyController::isMovementAllowed() const {
    if (_safetyState == SafetyState::ESTOP_ACTIVE) return false;
    if (_sensors.isEStopTriggered()) return false;
    return true;
}

void SafetyController::heartbeatReceived() {
    _lastHeartbeat = millis();
}

bool SafetyController::resetEStop() {
    if (_sensors.isEStopTriggered()) {
        LOG_W(TAG, "Cannot reset E-stop — button still pressed");
        return false;
    }

    LOG_I(TAG, "E-stop reset — returning to OK state");
    _safetyState = SafetyState::OK;
    _lastError = ErrorCode::NONE;
    return true;
}

void SafetyController::clearError() {
    if (_safetyState != SafetyState::ESTOP_ACTIVE) {
        _safetyState = SafetyState::OK;
        _lastError = ErrorCode::NONE;
    }
}

void SafetyController::startMoveWatchdog() {
    _moveWatchdog.start(MOTION_WATCHDOG_MS);
    _moveWatchdogActive = true;
}

void SafetyController::cancelMoveWatchdog() {
    _moveWatchdog.cancel();
    _moveWatchdogActive = false;
}

void SafetyController::_setError(ErrorCode code) {
    _lastError = code;
    if (_safetyState != SafetyState::ESTOP_ACTIVE) {
        _safetyState = SafetyState::ERROR;
    }
}
