#include "sensor_manager.h"

static const char* TAG = "SENSOR";

SensorManager::SensorManager()
    : _xHome(PIN_LIMIT_X_HOME, SWITCH_ACTIVE_LOW),
      _yHome(PIN_LIMIT_Y_HOME, SWITCH_ACTIVE_LOW),
      _zTop(PIN_LIMIT_Z_TOP, SWITCH_ACTIVE_LOW),
      _zBottom(PIN_LIMIT_Z_BOTTOM, SWITCH_ACTIVE_LOW),
      _estop(PIN_ESTOP, ESTOP_ACTIVE_LOW) {}

void SensorManager::begin() {
    _xHome.begin();
    _yHome.begin();
    _zTop.begin();
    _zBottom.begin();
    _estop.begin();
    _prize.begin();

    LOG_I(TAG, "All sensors initialized");
    LOG_I(TAG, "  X-Home: pin %d", _xHome.getPin());
    LOG_I(TAG, "  Y-Home: pin %d", _yHome.getPin());
    LOG_I(TAG, "  Z-Top:  pin %d", _zTop.getPin());
    LOG_I(TAG, "  Z-Bot:  pin %d", _zBottom.getPin());
    LOG_I(TAG, "  E-Stop: pin %d", _estop.getPin());
}

void SensorManager::update() {
    _xHome.update();
    _yHome.update();
    _zTop.update();
    _zBottom.update();
    _estop.update();
    _prize.update();
}

uint16_t SensorManager::getSensorBitmask() const {
    uint16_t mask = 0;
    if (_xHome.isTriggered())    mask |= (1 << 0);
    if (_yHome.isTriggered())    mask |= (1 << 1);
    if (_zTop.isTriggered())     mask |= (1 << 2);
    if (_zBottom.isTriggered())  mask |= (1 << 3);
    if (_estop.isTriggered())    mask |= (1 << 4);
    if (_prize.isBeamBroken())   mask |= (1 << 5);
    if (_prize.prizeDetected())  mask |= (1 << 6);
    return mask;
}
