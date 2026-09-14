#pragma once
// =============================================================
// Non-Blocking Timer Utility
// =============================================================

#ifndef TIMER_UTIL_H
#define TIMER_UTIL_H

#include <Arduino.h>

class NonBlockingTimer {
public:
    NonBlockingTimer(unsigned long intervalMs = 0)
        : _interval(intervalMs), _lastTick(0), _running(false) {}

    void setInterval(unsigned long ms) { _interval = ms; }

    void start() {
        _lastTick = millis();
        _running = true;
    }

    void stop() { _running = false; }

    void reset() { _lastTick = millis(); }

    bool isRunning() const { return _running; }

    // Returns true once per interval, then auto-resets
    bool elapsed() {
        if (!_running || _interval == 0) return false;
        unsigned long now = millis();
        if (now - _lastTick >= _interval) {
            _lastTick = now;
            return true;
        }
        return false;
    }

    // Returns elapsed time since start/reset
    unsigned long elapsedMs() const {
        return millis() - _lastTick;
    }

    // Returns remaining time until next tick
    unsigned long remainingMs() const {
        if (!_running) return 0;
        unsigned long elapsed = millis() - _lastTick;
        if (elapsed >= _interval) return 0;
        return _interval - elapsed;
    }

private:
    unsigned long _interval;
    unsigned long _lastTick;
    bool _running;
};

// One-shot timer — fires once, then stops
class OneShotTimer {
public:
    OneShotTimer() : _deadline(0), _running(false) {}

    void start(unsigned long durationMs) {
        _deadline = millis() + durationMs;
        _running = true;
    }

    void cancel() { _running = false; }

    bool isRunning() const { return _running; }

    bool hasExpired() {
        if (!_running) return false;
        if (millis() >= _deadline) {
            _running = false;
            return true;
        }
        return false;
    }

    unsigned long remainingMs() const {
        if (!_running) return 0;
        unsigned long now = millis();
        if (now >= _deadline) return 0;
        return _deadline - now;
    }

private:
    unsigned long _deadline;
    bool _running;
};

#endif // TIMER_UTIL_H
