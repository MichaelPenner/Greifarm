#include "game_state_machine.h"

static const char* TAG = "GAME";

GameStateMachine::GameStateMachine(MotionPlanner& motion, HomingController& homing,
                                     ClawController& claw, SensorManager& sensors,
                                     SafetyController& safety)
    : _motion(motion), _homing(homing), _claw(claw),
      _sensors(sensors), _safety(safety),
      _state(MachineState::BOOT), _previousState(MachineState::BOOT),
      _stateChangeCb(nullptr), _gameEventCb(nullptr) {
    memset(&_stats, 0, sizeof(_stats));
}

void GameStateMachine::begin() {
    _stats.credits = machineConfig.settings().free_play ? 999 : 0;
    _setState(MachineState::INITIALIZING);
    LOG_I(TAG, "Game engine started — free_play=%s",
          machineConfig.settings().free_play ? "yes" : "no");
}

void GameStateMachine::update() {
    // 1. Process new coins
    int newCoins = _sensors.coinAcceptor().getPendingCoins();
    if (newCoins > 0) {
        addCredits(newCoins);
        _emitGameEvent("coin_inserted", "");
    }

    // E-stop override — can happen from any state
    if (_safety.isEStopActive() && _state != MachineState::EMERGENCY_STOP) {
        _previousState = _state;
        _setState(MachineState::EMERGENCY_STOP);
        return;
    }

    // Safety error override
    if (_safety.getState() == SafetyState::ERROR &&
        _state != MachineState::ERROR_STATE &&
        _state != MachineState::EMERGENCY_STOP &&
        _state != MachineState::MAINTENANCE) {
        _setState(MachineState::ERROR_STATE);
        return;
    }

    switch (_state) {
        case MachineState::BOOT:           _handleBoot(); break;
        case MachineState::INITIALIZING:   _handleInitializing(); break;
        case MachineState::HOMING:         _handleHoming(); break;
        case MachineState::IDLE:           _handleIdle(); break;
        case MachineState::READY:          _handleReady(); break;
        case MachineState::PLAYING:        _handlePlaying(); break;
        case MachineState::CLAW_DESCENDING: _handleClawDescending(); break;
        case MachineState::CLAW_GRABBING:  _handleClawGrabbing(); break;
        case MachineState::CLAW_RETRACTING: _handleClawRetracting(); break;
        case MachineState::RETURNING_TO_DROP: _handleReturningToDrop(); break;
        case MachineState::RELEASING:      _handleReleasing(); break;
        case MachineState::CHECKING_PRIZE: _handleCheckingPrize(); break;
        case MachineState::WIN:            _handleWin(); break;
        case MachineState::GAME_OVER:      _handleGameOver(); break;
        case MachineState::ERROR_STATE:    _handleError(); break;
        case MachineState::EMERGENCY_STOP: _handleEStop(); break;
        case MachineState::MAINTENANCE:    _handleMaintenance(); break;
        case MachineState::CALIBRATION:    break; // Handled externally
    }
}

void GameStateMachine::_setState(MachineState newState) {
    if (newState == _state) return;

    MachineState old = _state;
    _state = newState;

    LOG_I(TAG, "State: %s → %s", getStateString(), "");  // Will log new state
    LOG_I(TAG, "New state: %s", getStateString());

    if (_stateChangeCb) {
        _stateChangeCb(old, newState);
    }
}

void GameStateMachine::_handleBoot() {
    // Should transition quickly to INITIALIZING
    _setState(MachineState::INITIALIZING);
}

void GameStateMachine::_handleInitializing() {
    // Start homing if configured
    // TEMPORARY OVERRIDE: Force disable auto-home because endstops are not connected
    // This ignores the NVS setting so you can test right now.
    if (false /* machineConfig.settings().auto_home_on_boot */) {
        _homing.startFullHoming();
        _setState(MachineState::HOMING);
    } else {
        // If auto-home is disabled (usually for testing without endstops),
        // we assume the current position is (0,0,0) so movement is allowed.
        _motion.setPositionXY(0, 0);
        _motion.setPositionZ(0);
        _motion.setPositionState(PositionState::HOMED);
        _setState(MachineState::IDLE);
    }
}

void GameStateMachine::_handleHoming() {
    if (_homing.update()) {
        // Still homing
        return;
    }

    if (_homing.isComplete()) {
        LOG_I(TAG, "Homing complete");
        _homing.reset();
        _setState(MachineState::IDLE);
    } else if (_homing.hasFailed()) {
        LOG_E(TAG, "Homing failed");
        _homing.reset();
        _setState(MachineState::ERROR_STATE);
    }
}

void GameStateMachine::_handleIdle() {
    // Check if we can transition to READY
    if (_canStartGame()) {
        _setState(MachineState::READY);
    }
}

void GameStateMachine::_handleReady() {
    // Wait for game start command
    if (!_canStartGame()) {
        _setState(MachineState::IDLE);
    }
}

void GameStateMachine::_handlePlaying() {
    const auto& joy = machineConfig.settings();

    // Check game timer
    if (_gameTimer.hasExpired()) {
        LOG_I(TAG, "Game time expired — triggering auto-grab");
        triggerGrab();
        return;
    }

    // Update game time remaining
    _stats.gameTimeRemaining = _gameTimer.remainingMs();
}

void GameStateMachine::_handleClawDescending() {
    const auto& cfg = machineConfig.settings();

    // Check if Z has reached bottom or limit switch
    if (!_motion.isMoving() || _sensors.isZBottomTriggered()) {
        _motion.stop();
        LOG_I(TAG, "Claw at bottom — closing");
        _claw.closeWithStrength(cfg.claw_strength);
        _setState(MachineState::CLAW_GRABBING);
    }
}

void GameStateMachine::_handleClawGrabbing() {
    _claw.update();

    if (_claw.isGripComplete()) {
        _claw.clearGripComplete();
        LOG_I(TAG, "Grip complete — retracting");
        _motion.moveToZ(0); // Move Z to top
        _setState(MachineState::CLAW_RETRACTING);
    }
}

void GameStateMachine::_handleClawRetracting() {
    if (!_motion.isMoving() || _sensors.isZTopTriggered()) {
        _motion.stop();
        LOG_I(TAG, "Claw retracted — returning to drop zone");

        const auto& cfg = machineConfig.settings();
        _motion.moveToXY(cfg.drop_x, cfg.drop_y);
        _setState(MachineState::RETURNING_TO_DROP);
    }
}

void GameStateMachine::_handleReturningToDrop() {
    if (!_motion.isMoving()) {
        LOG_I(TAG, "At drop zone — releasing");
        _claw.open();
        _setState(MachineState::RELEASING);
        _stateTimer.start(1000); // Wait 1 second for claw to open fully
    }
}

void GameStateMachine::_handleReleasing() {
    _claw.update();

    if (_stateTimer.hasExpired()) {
        LOG_I(TAG, "Claw opened — checking for prize");
        _sensors.prizeSensor().setEnabled(true);
        _sensors.prizeSensor().resetCooldown();
        _prizeCheckTimer.start(PRIZE_CHECK_TIMEOUT_MS);
        _setState(MachineState::CHECKING_PRIZE);
    }
}

void GameStateMachine::_handleCheckingPrize() {
    if (_sensors.isPrizeDetected()) {
        _sensors.prizeSensor().clearEvent();
        _sensors.prizeSensor().setEnabled(false);
        _stats.prizeDetected = true;
        _stats.gamesWon++;
        _setState(MachineState::WIN);
        _emitGameEvent("win", "");
        _stateTimer.start(5000); // Victory display time
        return;
    }

    if (_prizeCheckTimer.hasExpired()) {
        _sensors.prizeSensor().setEnabled(false);
        _stats.prizeDetected = false;
        _setState(MachineState::GAME_OVER);
        _emitGameEvent("loss", "");
        _stateTimer.start(3000); // Game over display time
    }
}

void GameStateMachine::_handleWin() {
    if (_stateTimer.hasExpired()) {
        LOG_I(TAG, "Victory sequence complete");
        _endGame();
    }
}

void GameStateMachine::_handleGameOver() {
    if (_stateTimer.hasExpired()) {
        LOG_I(TAG, "Game over sequence complete");
        _endGame();
    }
}

void GameStateMachine::_handleError() {
    // Wait for error to be cleared
    if (_safety.isOk()) {
        _setState(MachineState::IDLE);
    }
}

void GameStateMachine::_handleEStop() {
    // Wait for E-stop to be released and reset
    // This is handled by external resetEStop() call
}

void GameStateMachine::_handleMaintenance() {
    // Maintenance mode — all manual control is allowed
    // Exit via exitMaintenance()
}

// === External triggers ===

bool GameStateMachine::startGame(const char* playerName) {
    if (_state != MachineState::READY) {
        LOG_W(TAG, "Cannot start game — not in READY state (current: %s)", getStateString());
        return false;
    }

    if (!_canStartGame()) {
        LOG_W(TAG, "Cannot start game — preconditions not met");
        return false;
    }

    const auto& cfg = machineConfig.settings();

    // Deduct credits
    if (!cfg.free_play) {
        if (_stats.credits < cfg.credits_per_game) {
            LOG_W(TAG, "Not enough credits: %d < %d", _stats.credits, cfg.credits_per_game);
            return false;
        }
        _stats.credits -= cfg.credits_per_game;
    }

    _stats.gamesPlayed++;
    _stats.currentGameNumber++;
    _stats.playerName = playerName;
    _stats.prizeDetected = false;
    _stats.gameStartTime = millis();

    // Start game timer
    _gameTimer.start(cfg.game_duration_s * 1000UL);
    _stats.gameTimeRemaining = cfg.game_duration_s * 1000UL;

    // Enable motors and open claw
    _motion.steppers().enableMotors();
    _claw.open();

    _setState(MachineState::PLAYING);
    _emitGameEvent("game_start", playerName);

    LOG_I(TAG, "🎮 Game started! Player: %s, Duration: %ds",
          playerName, cfg.game_duration_s);
    return true;
}

bool GameStateMachine::stopGame() {
    if (!isGameActive()) return false;

    LOG_I(TAG, "Game stopped manually");
    _motion.stop();
    _claw.open();
    _setState(MachineState::GAME_OVER);
    _stateTimer.start(2000);
    return true;
}

bool GameStateMachine::triggerGrab() {
    if (_state != MachineState::PLAYING) {
        LOG_W(TAG, "Cannot grab — not in PLAYING state");
        return false;
    }

    LOG_I(TAG, "🎯 GRAB triggered — descending");
    _motion.setJoystickSpeed(0, 0, 0); // Stop XY movement

    // Descend Z to maximum depth
    const auto& cfg = machineConfig.settings();
    _motion.moveToZ(cfg.z_max_mm);
    _setState(MachineState::CLAW_DESCENDING);
    return true;
}

bool GameStateMachine::addCredits(int count) {
    _stats.credits += count;
    LOG_I(TAG, "Credits added: %d (total: %d)", count, _stats.credits);

    // Check if we should transition to READY
    if (_state == MachineState::IDLE && _canStartGame()) {
        _setState(MachineState::READY);
    }
    return true;
}

bool GameStateMachine::enterMaintenance() {
    if (isGameActive()) {
        LOG_W(TAG, "Cannot enter maintenance — game active");
        return false;
    }
    _setState(MachineState::MAINTENANCE);
    _motion.steppers().enableMotors();
    return true;
}

bool GameStateMachine::exitMaintenance() {
    if (_state != MachineState::MAINTENANCE) return false;
    _setState(MachineState::IDLE);
    return true;
}

bool GameStateMachine::enterCalibration() {
    if (isGameActive()) return false;
    _setState(MachineState::CALIBRATION);
    _motion.steppers().enableMotors();
    return true;
}

bool GameStateMachine::exitCalibration() {
    if (_state != MachineState::CALIBRATION) return false;
    _setState(MachineState::IDLE);
    return true;
}

bool GameStateMachine::_canStartGame() const {
    const auto& cfg = machineConfig.settings();

    if (!_safety.isMovementAllowed()) return false;
    if (_motion.getPositionState() == PositionState::UNKNOWN) return false;
    if (!cfg.free_play && _stats.credits < cfg.credits_per_game) return false;

    return true;
}

bool GameStateMachine::isGameActive() const {
    return (_state == MachineState::PLAYING ||
            _state == MachineState::CLAW_DESCENDING ||
            _state == MachineState::CLAW_GRABBING ||
            _state == MachineState::CLAW_RETRACTING ||
            _state == MachineState::RETURNING_TO_DROP ||
            _state == MachineState::RELEASING ||
            _state == MachineState::CHECKING_PRIZE);
}

unsigned long GameStateMachine::getGameTimeRemainingMs() const {
    if (!isGameActive()) return 0;
    return _gameTimer.remainingMs();
}

const char* GameStateMachine::getStateString() const {
    switch (_state) {
        case MachineState::BOOT:              return "BOOT";
        case MachineState::INITIALIZING:      return "INITIALIZING";
        case MachineState::HOMING:            return "HOMING";
        case MachineState::IDLE:              return "IDLE";
        case MachineState::READY:             return "READY";
        case MachineState::PLAYING:           return "PLAYING";
        case MachineState::CLAW_DESCENDING:   return "CLAW_DESCENDING";
        case MachineState::CLAW_GRABBING:     return "CLAW_GRABBING";
        case MachineState::CLAW_RETRACTING:   return "CLAW_RETRACTING";
        case MachineState::RETURNING_TO_DROP: return "RETURNING";
        case MachineState::RELEASING:         return "RELEASING";
        case MachineState::CHECKING_PRIZE:    return "CHECKING_PRIZE";
        case MachineState::WIN:               return "WIN";
        case MachineState::GAME_OVER:         return "GAME_OVER";
        case MachineState::ERROR_STATE:       return "ERROR";
        case MachineState::EMERGENCY_STOP:    return "EMERGENCY_STOP";
        case MachineState::MAINTENANCE:       return "MAINTENANCE";
        case MachineState::CALIBRATION:       return "CALIBRATION";
        default:                              return "UNKNOWN";
    }
}

void GameStateMachine::_emitGameEvent(const char* event, const char* data) {
    if (_gameEventCb) {
        _gameEventCb(event, data);
    }
}

void GameStateMachine::_endGame() {
    // Re-home if configured
    // TEMPORARY OVERRIDE: Force disable re-homing because endstops are not connected
    if (false /* machineConfig.settings().re_home_between_games */) {
        _homing.startFullHoming();
        _setState(MachineState::HOMING);
    } else {
        // Just return to park and go idle
        const auto& cfg = machineConfig.settings();
        _motion.moveToXY(cfg.park_x, cfg.park_y);
        _setState(MachineState::IDLE);
    }
}
