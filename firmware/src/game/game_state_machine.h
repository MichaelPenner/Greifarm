#pragma once
// =============================================================
// Game State Machine — Arcade Claw Machine Game Engine
// =============================================================

#ifndef GAME_STATE_MACHINE_H
#define GAME_STATE_MACHINE_H

#include <Arduino.h>
#include "../motion/motion_planner.h"
#include "../motion/homing.h"
#include "../claw/claw_controller.h"
#include "../sensors/sensor_manager.h"
#include "../safety/safety_controller.h"
#include "../config/config_manager.h"
#include "../utils/logger.h"
#include "../utils/timer.h"

enum class MachineState : uint8_t {
    BOOT,
    INITIALIZING,
    HOMING,
    IDLE,
    READY,
    PLAYING,
    CLAW_DESCENDING,
    CLAW_GRABBING,
    CLAW_RETRACTING,
    RETURNING_TO_DROP,
    RELEASING,
    CHECKING_PRIZE,
    WIN,
    GAME_OVER,
    ERROR_STATE,
    EMERGENCY_STOP,
    MAINTENANCE,
    CALIBRATION
};

// Callback for state changes (used to notify communication layer)
using StateChangeCallback = void(*)(MachineState oldState, MachineState newState);
using GameEventCallback = void(*)(const char* event, const char* data);

struct GameStats {
    int credits;
    int gamesPlayed;
    int gamesWon;
    int currentGameNumber;
    unsigned long gameStartTime;
    unsigned long gameTimeRemaining;
    bool prizeDetected;
    String playerName;
};

class GameStateMachine {
public:
    GameStateMachine(MotionPlanner& motion, HomingController& homing,
                     ClawController& claw, SensorManager& sensors,
                     SafetyController& safety);

    void begin();

    // Must be called every loop iteration
    void update();

    // External triggers
    bool startGame(const char* playerName = "Player");
    bool stopGame();
    bool triggerGrab();   // Player presses the grab button
    bool addCredits(int count);
    bool enterMaintenance();
    bool exitMaintenance();
    bool enterCalibration();
    bool exitCalibration();

    // State access
    MachineState getState() const { return _state; }
    const char* getStateString() const;
    const GameStats& getStats() const { return _stats; }
    int getCredits() const { return _stats.credits; }
    unsigned long getGameTimeRemainingMs() const;
    bool isGameActive() const;

    // Callbacks
    void onStateChange(StateChangeCallback cb) { _stateChangeCb = cb; }
    void onGameEvent(GameEventCallback cb) { _gameEventCb = cb; }

private:
    MotionPlanner& _motion;
    HomingController& _homing;
    ClawController& _claw;
    SensorManager& _sensors;
    SafetyController& _safety;

    MachineState _state;
    MachineState _previousState; // For E-stop recovery
    GameStats _stats;

    StateChangeCallback _stateChangeCb;
    GameEventCallback _gameEventCb;

    OneShotTimer _gameTimer;
    OneShotTimer _stateTimer;   // Generic timer for state transitions
    OneShotTimer _prizeCheckTimer;

    void _setState(MachineState newState);
    void _handleBoot();
    void _handleInitializing();
    void _handleHoming();
    void _handleIdle();
    void _handleReady();
    void _handlePlaying();
    void _handleClawDescending();
    void _handleClawGrabbing();
    void _handleClawRetracting();
    void _handleReturningToDrop();
    void _handleReleasing();
    void _handleCheckingPrize();
    void _handleWin();
    void _handleGameOver();
    void _handleError();
    void _handleEStop();
    void _handleMaintenance();
    void _endGame();

    bool _canStartGame() const;
    void _emitGameEvent(const char* event, const char* data = "");
};

#endif // GAME_STATE_MACHINE_H
