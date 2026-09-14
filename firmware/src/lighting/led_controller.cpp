#include "led_controller.h"
#include "../utils/logger.h"

static const char* TAG = "LED";

LedController::LedController()
    : _numLeds(DEFAULT_LED_COUNT), _currentScene(LedScene::OFF),
      _animStep(0), _hue(0), _animTimer(LED_UPDATE_INTERVAL_MS) {}

void LedController::begin() {
    _numLeds = min(machineConfig.settings().led_count, MAX_LEDS);

    FastLED.addLeds<LED_CHIPSET, PIN_WS2812_DATA, LED_COLOR_ORDER>(_leds, _numLeds);
    FastLED.setBrightness(machineConfig.settings().led_brightness);
    FastLED.setMaxPowerInMilliWatts(DEFAULT_LED_MAX_POWER_MW);

    clear();
    _animTimer.start();

    LOG_I(TAG, "LED controller initialized: %d LEDs, brightness=%d",
          _numLeds, machineConfig.settings().led_brightness);
}

void LedController::update() {
    if (!_animTimer.elapsed()) return;

    switch (_currentScene) {
        case LedScene::OFF:           clear(); break;
        case LedScene::BOOT:          _animBoot(); break;
        case LedScene::IDLE:          _animIdle(); break;
        case LedScene::READY:         _animReady(); break;
        case LedScene::GAME_START:    _animGameStart(); break;
        case LedScene::PLAYING:       _animPlaying(); break;
        case LedScene::CLAW_CLOSE:    _animPlaying(); break;
        case LedScene::PRIZE_DETECTED:_animPrizeDetected(); break;
        case LedScene::WIN:           _animWin(); break;
        case LedScene::LOSS:          _animLoss(); break;
        case LedScene::ERROR_SCENE:   _animError(); break;
        case LedScene::EMERGENCY_STOP:_animEStop(); break;
        case LedScene::MAINTENANCE:   setAllColor(CRGB(40, 40, 80)); break;
        case LedScene::RAINBOW:       _animRainbow(); break;
        case LedScene::BREATHING:     _animBreathing(); break;
        case LedScene::CHASE:         _animChase(); break;
        case LedScene::CUSTOM:        break; // Manual pixel control
    }

    FastLED.show();
    _animStep++;
}

void LedController::setScene(LedScene scene) {
    if (scene == _currentScene) return;
    _currentScene = scene;
    _animStep = 0;
    _hue = 0;
    LOG_I(TAG, "Scene: %s", getSceneName());
}

void LedController::setBrightness(uint8_t brightness) {
    FastLED.setBrightness(brightness);
}

void LedController::setAllColor(CRGB color) {
    fill_solid(_leds, _numLeds, color);
}

void LedController::setPixel(int index, CRGB color) {
    if (index >= 0 && index < _numLeds) {
        _leds[index] = color;
    }
}

void LedController::clear() {
    fill_solid(_leds, _numLeds, CRGB::Black);
    FastLED.show();
}

const char* LedController::getSceneName() const {
    switch (_currentScene) {
        case LedScene::OFF:            return "off";
        case LedScene::BOOT:           return "boot";
        case LedScene::IDLE:           return "idle";
        case LedScene::READY:          return "ready";
        case LedScene::GAME_START:     return "game_start";
        case LedScene::PLAYING:        return "playing";
        case LedScene::CLAW_CLOSE:     return "claw_close";
        case LedScene::PRIZE_DETECTED: return "prize_detected";
        case LedScene::WIN:            return "win";
        case LedScene::LOSS:           return "loss";
        case LedScene::ERROR_SCENE:    return "error";
        case LedScene::EMERGENCY_STOP: return "emergency_stop";
        case LedScene::MAINTENANCE:    return "maintenance";
        case LedScene::RAINBOW:        return "rainbow";
        case LedScene::BREATHING:      return "breathing";
        case LedScene::CHASE:          return "chase";
        case LedScene::CUSTOM:         return "custom";
        default:                       return "unknown";
    }
}

LedScene LedController::sceneFromName(const char* name) {
    if (strcmp(name, "off") == 0) return LedScene::OFF;
    if (strcmp(name, "boot") == 0) return LedScene::BOOT;
    if (strcmp(name, "idle") == 0) return LedScene::IDLE;
    if (strcmp(name, "ready") == 0) return LedScene::READY;
    if (strcmp(name, "game_start") == 0) return LedScene::GAME_START;
    if (strcmp(name, "playing") == 0) return LedScene::PLAYING;
    if (strcmp(name, "claw_close") == 0) return LedScene::CLAW_CLOSE;
    if (strcmp(name, "prize_detected") == 0) return LedScene::PRIZE_DETECTED;
    if (strcmp(name, "win") == 0) return LedScene::WIN;
    if (strcmp(name, "loss") == 0) return LedScene::LOSS;
    if (strcmp(name, "error") == 0) return LedScene::ERROR_SCENE;
    if (strcmp(name, "emergency_stop") == 0) return LedScene::EMERGENCY_STOP;
    if (strcmp(name, "maintenance") == 0) return LedScene::MAINTENANCE;
    if (strcmp(name, "rainbow") == 0) return LedScene::RAINBOW;
    if (strcmp(name, "breathing") == 0) return LedScene::BREATHING;
    if (strcmp(name, "chase") == 0) return LedScene::CHASE;
    return LedScene::OFF;
}

// === Animation implementations ===

void LedController::_animBoot() {
    // Soft blue sweep
    fadeToBlackBy(_leds, _numLeds, 20);
    int pos = _animStep % _numLeds;
    _leds[pos] = CRGB(100, 180, 255); // Light blue
}

void LedController::_animIdle() {
    // Gentle breathing light blue
    uint8_t val = beatsin8(15, 20, 100); // Slow breathing
    fill_solid(_leds, _numLeds, CRGB(val / 3, val / 2, val));
}

void LedController::_animReady() {
    // Brighter pulsing blue-white, inviting
    uint8_t val = beatsin8(30, 60, 200);
    fill_solid(_leds, _numLeds, CRGB(val / 2, val * 3 / 4, val));
}

void LedController::_animGameStart() {
    // Quick flash sequence
    if (_animStep % 10 < 5) {
        fill_solid(_leds, _numLeds, CRGB(100, 200, 255));
    } else {
        fill_solid(_leds, _numLeds, CRGB(255, 255, 255));
    }
}

void LedController::_animPlaying() {
    // Dynamic rotating rainbow at low saturation (playful)
    _hue++;
    fill_rainbow(_leds, _numLeds, _hue, 3);
    // Reduce saturation for a softer look
    for (int i = 0; i < _numLeds; i++) {
        _leds[i].fadeToBlackBy(100);
        _leds[i] += CRGB(30, 50, 80); // Add blue tint
    }
}

void LedController::_animPrizeDetected() {
    // Rapid gold/yellow flash
    if (_animStep % 6 < 3) {
        fill_solid(_leds, _numLeds, CRGB(255, 200, 0));
    } else {
        fill_solid(_leds, _numLeds, CRGB(255, 255, 200));
    }
}

void LedController::_animWin() {
    // Celebration! Fast rainbow + sparkle
    _hue += 5;
    fill_rainbow(_leds, _numLeds, _hue, 7);
    // Add random sparkles
    if (random8() < 80) {
        _leds[random16(_numLeds)] = CRGB::White;
    }
}

void LedController::_animLoss() {
    // Slow fade to dim blue
    uint8_t val = beatsin8(10, 10, 60);
    fill_solid(_leds, _numLeds, CRGB(val / 4, val / 3, val));
}

void LedController::_animError() {
    // Amber/orange pulsing
    uint8_t val = beatsin8(40, 50, 255);
    fill_solid(_leds, _numLeds, CRGB(val, val / 3, 0));
}

void LedController::_animEStop() {
    // RED flashing — urgent
    if (_animStep % 10 < 5) {
        fill_solid(_leds, _numLeds, CRGB(255, 0, 0));
    } else {
        fill_solid(_leds, _numLeds, CRGB::Black);
    }
}

void LedController::_animRainbow() {
    _hue++;
    fill_rainbow(_leds, _numLeds, _hue, 3);
}

void LedController::_animBreathing() {
    uint8_t val = beatsin8(20, 10, 255);
    fill_solid(_leds, _numLeds, CRGB(val / 3, val / 2, val)); // Blue breathing
}

void LedController::_animChase() {
    fadeToBlackBy(_leds, _numLeds, 40);
    int pos = (_animStep * 2) % _numLeds;
    _leds[pos] = CRGB(100, 180, 255);
    _leds[(pos + _numLeds / 3) % _numLeds] = CRGB(100, 255, 180);
    _leds[(pos + 2 * _numLeds / 3) % _numLeds] = CRGB(180, 100, 255);
}
