#include "claw_controller.h"

static const char* TAG = "CLAW";

ClawController::ClawController()
    : _state(ClawState::UNKNOWN), _currentAngle(0), _targetAngle(0),
      _attached(false), _gripComplete(false) {}

void ClawController::begin() {
    attach();
    open(); // Start in open position for safety
    LOG_I(TAG, "Claw initialized — open angle=%d, close angle=%d",
          machineConfig.settings().claw_open_angle,
          machineConfig.settings().claw_close_angle);
}

void ClawController::attach() {
    if (!_attached) {
        _servo.setPeriodHertz(SERVO_PWM_FREQ_HZ);
        _servo.attach(PIN_CLAW_SERVO, SERVO_MIN_PULSE_US, SERVO_MAX_PULSE_US);
        _attached = true;
    }
}

void ClawController::detach() {
    if (_attached) {
        _servo.detach();
        _attached = false;
    }
}

void ClawController::open() {
    int angle = machineConfig.settings().claw_open_angle;
    setAngle(angle);
    _state = ClawState::MOVING;
    _moveTimer.start(machineConfig.settings().claw_move_speed_ms);
    _gripComplete = false;
    LOG_I(TAG, "Opening claw → %d°", angle);
}

void ClawController::close() {
    int angle = machineConfig.settings().claw_close_angle;
    setAngle(angle);
    _state = ClawState::MOVING;
    _moveTimer.start(machineConfig.settings().claw_move_speed_ms);
    // Start grip timer after move completes (handled in update)
    LOG_I(TAG, "Closing claw → %d°", angle);
}

void ClawController::closeWithStrength(int strengthPercent) {
    const auto& cfg = machineConfig.settings();
    strengthPercent = constrain(strengthPercent, 0, 100);

    // Interpolate between open and close angle based on strength
    int range = cfg.claw_close_angle - cfg.claw_open_angle;
    int angle = cfg.claw_open_angle + (range * strengthPercent / 100);

    setAngle(angle);
    _state = ClawState::MOVING;
    _moveTimer.start(cfg.claw_move_speed_ms);
    LOG_I(TAG, "Closing claw at %d%% strength → %d°", strengthPercent, angle);
}

void ClawController::partial() {
    int angle = machineConfig.settings().claw_partial_angle;
    setAngle(angle);
    _state = ClawState::MOVING;
    _moveTimer.start(machineConfig.settings().claw_move_speed_ms);
    LOG_I(TAG, "Partial claw → %d°", angle);
}

void ClawController::setAngle(int angle) {
    angle = constrain(angle, 0, 180);
    _targetAngle = angle;
    attach();
    _servo.write(angle);
}

void ClawController::update() {
    if (_state == ClawState::MOVING && _moveTimer.hasExpired()) {
        // Movement complete
        _currentAngle = _targetAngle;

        const auto& cfg = machineConfig.settings();
        if (_currentAngle >= cfg.claw_close_angle - 5) {
            _state = ClawState::CLOSED;
            // Start grip delay timer
            _gripTimer.start(cfg.claw_grip_delay_ms);
        } else if (_currentAngle <= cfg.claw_open_angle + 5) {
            _state = ClawState::OPEN;
            _gripComplete = false;
        } else {
            _state = ClawState::PARTIAL;
        }
    }

    // Check grip delay completion
    if (_state == ClawState::CLOSED && _gripTimer.isRunning() && _gripTimer.hasExpired()) {
        _gripComplete = true;
        LOG_D(TAG, "Grip delay complete");
    }
}
