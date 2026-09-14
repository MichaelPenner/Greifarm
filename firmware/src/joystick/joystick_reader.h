#pragma once
// =============================================================
// Joystick Reader — ADC Reading + Calibration
// =============================================================

#ifndef JOYSTICK_READER_H
#define JOYSTICK_READER_H

#include <Arduino.h>
#include "../config/pin_config.h"
#include "../config/config_manager.h"

struct JoystickRaw {
    int x;
    int y;
    int z;
    bool button;
};

struct JoystickCalibrated {
    float x;    // -1.0 to +1.0
    float y;    // -1.0 to +1.0
    float z;    // -1.0 to +1.0
    bool button;
    bool buttonPressed;  // Edge-detected: true on press
    bool buttonReleased; // Edge-detected: true on release
};

class JoystickReader {
public:
    JoystickReader();

    void begin();

    // Read and process joystick — call at JOY_READ_INTERVAL_MS
    void update();

    // Get latest readings
    const JoystickRaw& getRaw() const { return _raw; }
    const JoystickCalibrated& getCalibrated() const { return _calibrated; }

    // Calibration — call when joystick is centered
    void calibrateCenter();

    // Auto-detect min/max range
    void startRangeCalibration();
    void stopRangeCalibration();
    bool isCalibrating() const { return _calibrating; }

private:
    JoystickRaw _raw;
    JoystickCalibrated _calibrated;
    bool _lastButton;
    bool _calibrating;
    int _calMinX, _calMaxX;
    int _calMinY, _calMaxY;
    int _calMinZ, _calMaxZ;

    int _readADC(uint8_t pin);
    float _normalize(int value, int center, int minVal, int maxVal,
                     int deadzone, bool invert, float expo);
};

#endif // JOYSTICK_READER_H
