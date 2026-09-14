#include "joystick_reader.h"
#include "../utils/logger.h"
#include <cmath>

static const char* TAG = "JOYSTK";

JoystickReader::JoystickReader()
    : _lastButton(false), _calibrating(false),
      _calMinX(0), _calMaxX(4095), _calMinY(0), _calMaxY(4095),
      _calMinZ(0), _calMaxZ(4095) {
    memset(&_raw, 0, sizeof(_raw));
    memset(&_calibrated, 0, sizeof(_calibrated));
}

void JoystickReader::begin() {
    // Configure ADC
    analogReadResolution(12);  // 12-bit: 0–4095
    analogSetAttenuation(ADC_11db);  // Full range: 0–3.3V

    // Button pin — input with pull-up (button connects to GND)
    pinMode(PIN_JOYSTICK_BTN, INPUT_PULLUP);

    LOG_I(TAG, "Joystick initialized on pins X=%d Y=%d Z=%d BTN=%d",
          PIN_JOYSTICK_X, PIN_JOYSTICK_Y, PIN_JOYSTICK_Z, PIN_JOYSTICK_BTN);
}

void JoystickReader::update() {
    // Read raw ADC values (averaged)
    _raw.x = _readADC(PIN_JOYSTICK_X);
    _raw.y = _readADC(PIN_JOYSTICK_Y);
    _raw.z = _readADC(PIN_JOYSTICK_Z);
    _raw.button = (digitalRead(PIN_JOYSTICK_BTN) == LOW); // Active LOW

    // Update range calibration if active
    if (_calibrating) {
        if (_raw.x < _calMinX) _calMinX = _raw.x;
        if (_raw.x > _calMaxX) _calMaxX = _raw.x;
        if (_raw.y < _calMinY) _calMinY = _raw.y;
        if (_raw.y > _calMaxY) _calMaxY = _raw.y;
        if (_raw.z < _calMinZ) _calMinZ = _raw.z;
        if (_raw.z > _calMaxZ) _calMaxZ = _raw.z;
    }

    const auto& cfg = machineConfig.settings();

    // Normalize to -1.0 … +1.0 with dead zone and expo curve
    _calibrated.x = _normalize(_raw.x, cfg.joy_x_center,
        cfg.joy_min, cfg.joy_max, cfg.joy_deadzone,
        cfg.joy_invert_x, cfg.joy_expo);

    _calibrated.y = _normalize(_raw.y, cfg.joy_y_center,
        cfg.joy_min, cfg.joy_max, cfg.joy_deadzone,
        cfg.joy_invert_y, cfg.joy_expo);

    _calibrated.z = _normalize(_raw.z, cfg.joy_z_center,
        cfg.joy_min, cfg.joy_max, cfg.joy_deadzone,
        cfg.joy_invert_z, cfg.joy_expo);

    // Apply sensitivity scaling
    _calibrated.x *= cfg.joy_sensitivity;
    _calibrated.y *= cfg.joy_sensitivity;
    _calibrated.z *= cfg.joy_sensitivity;

    // Button edge detection
    _calibrated.button = _raw.button;
    _calibrated.buttonPressed = (_raw.button && !_lastButton);
    _calibrated.buttonReleased = (!_raw.button && _lastButton);
    _lastButton = _raw.button;
}

int JoystickReader::_readADC(uint8_t pin) {
    // Average multiple samples to reduce noise
    int32_t sum = 0;
    for (int i = 0; i < JOY_ADC_SAMPLES; i++) {
        sum += analogRead(pin);
    }
    return static_cast<int>(sum / JOY_ADC_SAMPLES);
}

float JoystickReader::_normalize(int value, int center, int minVal, int maxVal,
                                  int deadzone, bool invert, float expo) {
    // Calculate offset from center
    int offset = value - center;

    // Apply dead zone
    if (abs(offset) < deadzone) {
        return 0.0f;
    }

    // Remove dead zone from calculation
    float normalized;
    if (offset > 0) {
        int range = maxVal - center - deadzone;
        if (range <= 0) return 0.0f;
        normalized = static_cast<float>(offset - deadzone) / static_cast<float>(range);
    } else {
        int range = center - minVal - deadzone;
        if (range <= 0) return 0.0f;
        normalized = static_cast<float>(offset + deadzone) / static_cast<float>(range);
    }

    // Clamp to -1.0 … +1.0
    normalized = constrain(normalized, -1.0f, 1.0f);

    // Apply exponential response curve
    // Preserves sign: sign(x) * |x|^expo
    if (expo != 1.0f) {
        float sign = (normalized >= 0) ? 1.0f : -1.0f;
        normalized = sign * powf(fabsf(normalized), expo);
    }

    // Apply inversion
    if (invert) normalized = -normalized;

    return normalized;
}

void JoystickReader::calibrateCenter() {
    // Read current position as the new center
    auto& cfg = machineConfig.settings();
    cfg.joy_x_center = _readADC(PIN_JOYSTICK_X);
    cfg.joy_y_center = _readADC(PIN_JOYSTICK_Y);
    cfg.joy_z_center = _readADC(PIN_JOYSTICK_Z);

    machineConfig.saveJoystick();
    LOG_I(TAG, "Center calibrated: X=%d Y=%d Z=%d",
          cfg.joy_x_center, cfg.joy_y_center, cfg.joy_z_center);
}

void JoystickReader::startRangeCalibration() {
    _calibrating = true;
    _calMinX = _calMinY = _calMinZ = 4095;
    _calMaxX = _calMaxY = _calMaxZ = 0;
    LOG_I(TAG, "Range calibration started — move joystick to all extremes");
}

void JoystickReader::stopRangeCalibration() {
    _calibrating = false;

    auto& cfg = machineConfig.settings();
    cfg.joy_min = min({_calMinX, _calMinY, _calMinZ});
    cfg.joy_max = max({_calMaxX, _calMaxY, _calMaxZ});

    machineConfig.saveJoystick();
    LOG_I(TAG, "Range calibration complete: min=%d max=%d", cfg.joy_min, cfg.joy_max);
}
