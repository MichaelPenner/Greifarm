#pragma once
// =============================================================
// Sensor Manager — Aggregate All Sensor States
// =============================================================

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "limit_switch.h"
#include "prize_sensor.h"
#include "../config/pin_config.h"
#include "../utils/logger.h"

class SensorManager {
public:
    SensorManager();

    void begin();
    void update();

    // Limit switch access
    bool isXHomeTriggered() const { return _xHome.isTriggered(); }
    bool isYHomeTriggered() const { return _yHome.isTriggered(); }
    bool isZTopTriggered() const { return _zTop.isTriggered(); }
    bool isZBottomTriggered() const { return _zBottom.isTriggered(); }

    // Emergency stop
    bool isEStopTriggered() const { return _estop.isTriggered(); }
    bool eStopJustTriggered() const { return _estop.justTriggered(); }

    // Prize sensor
    PrizeSensor& prizeSensor() { return _prize; }
    bool isPrizeDetected() const { return _prize.prizeDetected(); }

    // Get all sensor states as a bitmask for telemetry
    uint16_t getSensorBitmask() const;

    // Individual switch references (for homing, diagnostics)
    LimitSwitch& xHome() { return _xHome; }
    LimitSwitch& yHome() { return _yHome; }
    LimitSwitch& zTop() { return _zTop; }
    LimitSwitch& zBottom() { return _zBottom; }
    LimitSwitch& eStop() { return _estop; }

private:
    LimitSwitch _xHome;
    LimitSwitch _yHome;
    LimitSwitch _zTop;
    LimitSwitch _zBottom;
    LimitSwitch _estop;
    PrizeSensor _prize;
};

#endif // SENSOR_MANAGER_H
