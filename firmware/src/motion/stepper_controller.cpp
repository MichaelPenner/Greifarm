#include "stepper_controller.h"

StepperController::StepperController() : _motorsEnabled(false) {
    memset(_axes, 0, sizeof(_axes));
}

void StepperController::begin() {
    _initAxis(StepperAxis::MOTOR_A, PIN_MOTOR_A_STEP, PIN_MOTOR_A_DIR);
    _initAxis(StepperAxis::MOTOR_B, PIN_MOTOR_B_STEP, PIN_MOTOR_B_DIR);
    _initAxis(StepperAxis::MOTOR_Z, PIN_MOTOR_Z_STEP, PIN_MOTOR_Z_DIR);

    // Enable pin — shared for all drivers on CNC Shield
    pinMode(PIN_STEPPER_ENABLE, OUTPUT);
    disableMotors(); // Start disabled for safety

    // Set default speeds
    float defaultSpeed = DEFAULT_STEPS_PER_MM * DEFAULT_X_MAX_SPEED;
    float defaultAccel = DEFAULT_STEPS_PER_MM * DEFAULT_X_ACCELERATION;

    setMaxSpeed(StepperAxis::MOTOR_A, defaultSpeed);
    setMaxSpeed(StepperAxis::MOTOR_B, defaultSpeed);
    setMaxSpeed(StepperAxis::MOTOR_Z, DEFAULT_Z_STEPS_PER_MM * DEFAULT_Z_MAX_SPEED);

    setAcceleration(StepperAxis::MOTOR_A, defaultAccel);
    setAcceleration(StepperAxis::MOTOR_B, defaultAccel);
    setAcceleration(StepperAxis::MOTOR_Z, DEFAULT_Z_STEPS_PER_MM * DEFAULT_Z_ACCELERATION);
}

void StepperController::_initAxis(StepperAxis axis, uint8_t stepPin, uint8_t dirPin) {
    int idx = static_cast<int>(axis);
    _axes[idx].stepPin = stepPin;
    _axes[idx].dirPin = dirPin;
    _axes[idx].currentSteps = 0;
    _axes[idx].targetSteps = 0;
    _axes[idx].moving = false;
    _axes[idx].currentSpeed = 0;
    _axes[idx].stepsRemaining = 0;
    _axes[idx].direction = 1;

    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    digitalWrite(stepPin, LOW);
    digitalWrite(dirPin, LOW);
}

void StepperController::setMaxSpeed(StepperAxis axis, float stepsPerSec) {
    int idx = static_cast<int>(axis);
    _axes[idx].maxSpeed = stepsPerSec;
    _axes[idx].configuredMaxSpeed = stepsPerSec;
    _axes[idx].minInterval = 1000000.0f / stepsPerSec; // µs per step at max speed
}

void StepperController::setAcceleration(StepperAxis axis, float stepsPerSecSq) {
    int idx = static_cast<int>(axis);
    _axes[idx].acceleration = stepsPerSecSq;
    _axes[idx].configuredAcceleration = stepsPerSecSq;
}

void StepperController::enableMotors() {
    digitalWrite(PIN_STEPPER_ENABLE, LOW); // A4988: LOW = enabled
    _motorsEnabled = true;
}

void StepperController::disableMotors() {
    digitalWrite(PIN_STEPPER_ENABLE, HIGH); // A4988: HIGH = disabled
    _motorsEnabled = false;
}

void StepperController::moveTo(StepperAxis axis, int32_t targetSteps) {
    int idx = static_cast<int>(axis);
    StepperState& s = _axes[idx];

    s.maxSpeed = s.configuredMaxSpeed;
    s.acceleration = s.configuredAcceleration;
    s.minInterval = 1000000.0f / s.maxSpeed;
    s.targetSteps = targetSteps;
    _planMove(s);
}

void StepperController::moveRelative(StepperAxis axis, int32_t deltaSteps) {
    int idx = static_cast<int>(axis);
    moveTo(axis, _axes[idx].currentSteps + deltaSteps);
}

void StepperController::moveSynchronized(int32_t targetA, int32_t targetB) {
    // Restore base speeds
    _axes[0].maxSpeed = _axes[0].configuredMaxSpeed;
    _axes[0].acceleration = _axes[0].configuredAcceleration;
    _axes[0].minInterval = 1000000.0f / _axes[0].maxSpeed;

    _axes[1].maxSpeed = _axes[1].configuredMaxSpeed;
    _axes[1].acceleration = _axes[1].configuredAcceleration;
    _axes[1].minInterval = 1000000.0f / _axes[1].maxSpeed;

    // Plan both moves
    _axes[0].targetSteps = targetA;
    _axes[1].targetSteps = targetB;
    _planMove(_axes[0]);
    _planMove(_axes[1]);

    // For synchronized CoreXY moves, scale speeds so both axes
    // finish at the same time (Bresenham-style synchronization)
    int32_t stepsA = abs(targetA - _axes[0].currentSteps);
    int32_t stepsB = abs(targetB - _axes[1].currentSteps);

    if (stepsA > 0 && stepsB > 0) {
        // The axis with fewer steps should run slower proportionally
        float ratio;
        if (stepsA >= stepsB) {
            ratio = static_cast<float>(stepsB) / static_cast<float>(stepsA);
            // Motor A runs at full speed, Motor B runs at ratio × speed
            _axes[1].maxSpeed = _axes[0].maxSpeed * ratio;
            _axes[1].minInterval = 1000000.0f / _axes[1].maxSpeed;
            _axes[1].acceleration = _axes[0].acceleration * ratio;
        } else {
            ratio = static_cast<float>(stepsA) / static_cast<float>(stepsB);
            _axes[0].maxSpeed = _axes[1].maxSpeed * ratio;
            _axes[0].minInterval = 1000000.0f / _axes[0].maxSpeed;
            _axes[0].acceleration = _axes[1].acceleration * ratio;
        }
        // Re-plan with adjusted speeds
        _planMove(_axes[0]);
        _planMove(_axes[1]);
    }
}

void StepperController::_planMove(StepperState& s) {
    int32_t delta = s.targetSteps - s.currentSteps;

    if (delta == 0) {
        s.moving = false;
        s.stepsRemaining = 0;
        return;
    }

    s.direction = (delta > 0) ? 1 : -1;
    s.totalSteps = abs(delta);
    s.stepsRemaining = s.totalSteps;

    // Set direction pin
    digitalWrite(s.dirPin, (s.direction > 0) ? HIGH : LOW);

    // Calculate acceleration profile (trapezoidal)
    // accelSteps = v² / (2 * a)
    s.accelSteps = static_cast<int32_t>(
        (s.maxSpeed * s.maxSpeed) / (2.0f * s.acceleration)
    );

    // If we can't reach max speed, use triangular profile
    if (s.accelSteps > s.totalSteps / 2) {
        s.accelSteps = s.totalSteps / 2;
    }

    s.decelStart = s.totalSteps - s.accelSteps;

    // Start with a slow initial speed
    s.currentSpeed = sqrtf(2.0f * s.acceleration); // Speed after 1 step
    if (s.currentSpeed < 1.0f) s.currentSpeed = 1.0f;
    s.stepInterval = 1000000.0f / s.currentSpeed;

    s.lastStepTime = micros();
    s.moving = true;
}

bool StepperController::update() {
    bool anyMoving = false;
    unsigned long now = micros();

    for (int i = 0; i < 3; i++) {
        StepperState& s = _axes[i];
        if (!s.moving) continue;

        // Check if it's time for the next step
        if ((now - s.lastStepTime) >= static_cast<unsigned long>(s.stepInterval)) {
            _step(s);
            anyMoving = true;
        } else {
            anyMoving = true;
        }
    }

    return anyMoving;
}

void StepperController::_step(StepperState& s) {
    if (s.stepsRemaining <= 0) {
        s.moving = false;
        s.currentSpeed = 0;
        return;
    }

    // Generate step pulse (A4988 needs 1us, digitalWrite takes 1-2us, so no delay needed)
    digitalWrite(s.stepPin, HIGH);
    digitalWrite(s.stepPin, LOW);

    // Update position
    s.currentSteps += s.direction;
    s.stepsRemaining--;

    // Calculate steps completed
    int32_t stepsCompleted = s.totalSteps - s.stepsRemaining;

    // Update speed based on acceleration profile
    if (stepsCompleted < s.accelSteps) {
        // Accelerating
        s.currentSpeed = sqrtf(2.0f * s.acceleration * (stepsCompleted + 1));
        if (s.currentSpeed > s.maxSpeed) s.currentSpeed = s.maxSpeed;
    } else if (s.stepsRemaining <= s.accelSteps) {
        // Decelerating
        s.currentSpeed = sqrtf(2.0f * s.acceleration * s.stepsRemaining);
        if (s.currentSpeed < sqrtf(2.0f * s.acceleration)) {
            s.currentSpeed = sqrtf(2.0f * s.acceleration);
        }
    }
    // else: cruising at maxSpeed

    s.stepInterval = 1000000.0f / s.currentSpeed;
    if (s.stepInterval < STEP_INTERVAL_MIN_US) {
        s.stepInterval = STEP_INTERVAL_MIN_US;
    }

    s.lastStepTime = micros();

    // Check completion
    if (s.stepsRemaining <= 0) {
        s.moving = false;
        s.currentSpeed = 0;
    }
}

void StepperController::stopAll() {
    for (int i = 0; i < 3; i++) {
        if (_axes[i].moving) {
            // Plan deceleration stop
            int32_t decelSteps = static_cast<int32_t>(
                (_axes[i].currentSpeed * _axes[i].currentSpeed) /
                (2.0f * _axes[i].acceleration)
            );
            _axes[i].stepsRemaining = min((int32_t)_axes[i].stepsRemaining, decelSteps);
            _axes[i].decelStart = 0; // Start decelerating immediately
        }
    }
}

void StepperController::haltAll() {
    for (int i = 0; i < 3; i++) {
        _axes[i].moving = false;
        _axes[i].stepsRemaining = 0;
        _axes[i].currentSpeed = 0;
    }
}

int32_t StepperController::getPosition(StepperAxis axis) const {
    return _axes[static_cast<int>(axis)].currentSteps;
}

void StepperController::setPosition(StepperAxis axis, int32_t steps) {
    _axes[static_cast<int>(axis)].currentSteps = steps;
}

void StepperController::resetPosition(StepperAxis axis) {
    setPosition(axis, 0);
}

bool StepperController::isMoving(StepperAxis axis) const {
    return _axes[static_cast<int>(axis)].moving;
}

bool StepperController::isAnyMoving() const {
    return _axes[0].moving || _axes[1].moving || _axes[2].moving;
}
