#pragma once
// =============================================================
// LED Controller — WS2812B Lighting Engine
// =============================================================

#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>
#include <FastLED.h>
#include "../config/pin_config.h"
#include "../config/machine_config.h"
#include "../utils/timer.h"

// Maximum LED count (compile-time buffer)
#define MAX_LEDS 200

enum class LedScene : uint8_t {
    OFF,
    BOOT,
    IDLE,
    READY,
    GAME_START,
    PLAYING,
    CLAW_CLOSE,
    PRIZE_DETECTED,
    WIN,
    LOSS,
    ERROR_SCENE,
    EMERGENCY_STOP,
    MAINTENANCE,
    RAINBOW,
    BREATHING,
    CHASE,
    CUSTOM
};

class LedController {
public:
    LedController();

    void begin();
    void update();  // Non-blocking — call from main loop

    // Scene control
    void setScene(LedScene scene);
    LedScene getScene() const { return _currentScene; }
    const char* getSceneName() const;

    // Direct control
    void setBrightness(uint8_t brightness);
    void setAllColor(CRGB color);
    void setPixel(int index, CRGB color);
    void clear();

    // Scene name lookup
    static LedScene sceneFromName(const char* name);

private:
    CRGB _leds[MAX_LEDS];
    int _numLeds;
    LedScene _currentScene;
    NonBlockingTimer _animTimer;
    uint8_t _animStep;
    uint8_t _hue;

    // Animation methods (all non-blocking, called per frame)
    void _animBoot();
    void _animIdle();
    void _animReady();
    void _animPlaying();
    void _animWin();
    void _animLoss();
    void _animError();
    void _animEStop();
    void _animRainbow();
    void _animBreathing();
    void _animChase();
    void _animGameStart();
    void _animPrizeDetected();
};

#endif // LED_CONTROLLER_H
