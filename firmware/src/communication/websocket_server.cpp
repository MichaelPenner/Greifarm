#include "websocket_server.h"

static const char* TAG = "WS";

WebSocketServer::WebSocketServer(GameStateMachine& game, MotionPlanner& motion,
                                   HomingController& homing, ClawController& claw,
                                   SensorManager& sensors, SafetyController& safety,
                                   JoystickReader& joystick)
    : _server(WEBSOCKET_PORT), _ws("/ws"),
      _game(game), _motion(motion), _homing(homing),
      _claw(claw), _sensors(sensors), _safety(safety), _joystick(joystick),
      _telemetryTimer(TELEMETRY_INTERVAL_MS) {}

void WebSocketServer::begin() {
    _ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                       AwsEventType type, void* arg, uint8_t* data, size_t len) {
        _onWsEvent(server, client, type, arg, data, len);
    });

    _server.addHandler(&_ws);
    _server.begin();
    _telemetryTimer.start();

    LOG_I(TAG, "WebSocket server started on port %d", WEBSOCKET_PORT);
}

void WebSocketServer::update() {
    _ws.cleanupClients();

    // Send periodic telemetry
    if (_telemetryTimer.elapsed() && hasClients()) {
        sendTelemetry();
    }
}

void WebSocketServer::_onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                   AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            LOG_I(TAG, "Client connected: #%u from %s",
                  client->id(), client->remoteIP().toString().c_str());
            client->setCloseClientOnQueueFull(false);
            break;

        case WS_EVT_DISCONNECT:
            LOG_I(TAG, "Client disconnected: #%u", client->id());
            break;

        case WS_EVT_DATA:
            _handleMessage(client, data, len);
            break;

        case WS_EVT_ERROR:
            LOG_E(TAG, "WebSocket error from client #%u", client->id());
            break;

        case WS_EVT_PONG:
            _safety.heartbeatReceived();
            break;
    }
}

void WebSocketServer::_handleMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);

    if (err) {
        LOG_W(TAG, "JSON parse error: %s", err.c_str());
        _sendNack(client, "unknown", "Invalid JSON");
        return;
    }

    if (!doc.containsKey("type")) {
        _sendNack(client, "unknown", "Missing 'type' field");
        return;
    }

    _handleCommand(client, doc);
}

void WebSocketServer::_handleCommand(AsyncWebSocketClient* client, JsonDocument& doc) {
    const char* type = doc["type"];
    LOG_D(TAG, "Command: %s", type);

    // --- Heartbeat ---
    if (strcmp(type, Cmd::HEARTBEAT) == 0) {
        _safety.heartbeatReceived();
        JsonDocument resp;
        resp["type"] = Evt::HEARTBEAT_ACK;
        resp["uptime"] = millis();
        String out;
        serializeJson(resp, out);
        client->text(out);
        return;
    }

    // --- Movement ---
    if (strcmp(type, Cmd::MOVE) == 0) {
        if (!_safety.isMovementAllowed()) {
            _sendNack(client, type, "Movement not allowed");
            return;
        }
        float x = doc["x"] | 0.0f;
        float y = doc["y"] | 0.0f;
        float z = doc["z"] | 0.0f;
        _motion.setJoystickSpeed(x, y, z);
        return; // No ACK for continuous move commands (too frequent)
    }

    if (strcmp(type, Cmd::MOVE_TO) == 0) {
        if (!_safety.isMovementAllowed()) {
            _sendNack(client, type, "Movement not allowed");
            return;
        }
        float x = doc["x"] | -1.0f;
        float y = doc["y"] | -1.0f;
        float z = doc["z"] | -1.0f;

        if (x >= 0 && y >= 0) _motion.moveToXY(x, y);
        if (z >= 0) _motion.moveToZ(z);
        _sendAck(client, type);
        return;
    }

    if (strcmp(type, Cmd::MOVE_REL) == 0) {
        if (!_safety.isMovementAllowed()) {
            _sendNack(client, type, "Movement not allowed");
            return;
        }
        float dx = doc["dx"] | 0.0f;
        float dy = doc["dy"] | 0.0f;
        float dz = doc["dz"] | 0.0f;

        if (dx != 0 || dy != 0) _motion.moveRelativeXY(dx, dy);
        if (dz != 0) _motion.moveRelativeZ(dz);
        _sendAck(client, type);
        return;
    }

    // --- Homing ---
    if (strcmp(type, Cmd::HOME) == 0) {
        const char* axis = doc["axis"] | "all";
        if (strcmp(axis, "all") == 0) _homing.startFullHoming();
        else if (strcmp(axis, "x") == 0) _homing.startHomeX();
        else if (strcmp(axis, "y") == 0) _homing.startHomeY();
        else if (strcmp(axis, "z") == 0) _homing.startHomeZ();
        _sendAck(client, type);
        return;
    }

    // --- Stop ---
    if (strcmp(type, Cmd::STOP) == 0) {
        _motion.stop();
        _sendAck(client, type);
        return;
    }

    if (strcmp(type, Cmd::ESTOP) == 0) {
        _motion.emergencyStop();
        _sendAck(client, type);
        return;
    }

    if (strcmp(type, Cmd::RESET) == 0) {
        if (_safety.resetEStop()) {
            _safety.clearError();
            _sendAck(client, type);
        } else {
            _sendNack(client, type, "E-stop button still pressed");
        }
        return;
    }

    // --- Claw ---
    if (strcmp(type, Cmd::CLAW_OPEN) == 0) {
        _claw.open();
        _sendAck(client, type);
        return;
    }
    if (strcmp(type, Cmd::CLAW_CLOSE) == 0) {
        int strength = doc["strength"] | 100;
        _claw.closeWithStrength(strength);
        _sendAck(client, type);
        return;
    }
    if (strcmp(type, Cmd::CLAW_PARTIAL) == 0) {
        _claw.partial();
        _sendAck(client, type);
        return;
    }
    if (strcmp(type, Cmd::CLAW_ANGLE) == 0) {
        int angle = doc["angle"] | 90;
        _claw.setAngle(angle);
        _sendAck(client, type);
        return;
    }

    // --- Game ---
    if (strcmp(type, Cmd::GAME_START) == 0) {
        const char* player = doc["player"] | "Player";
        if (_game.startGame(player)) {
            _sendAck(client, type);
        } else {
            _sendNack(client, type, "Cannot start game");
        }
        return;
    }
    if (strcmp(type, Cmd::GAME_STOP) == 0) {
        _game.stopGame();
        _sendAck(client, type);
        return;
    }
    if (strcmp(type, Cmd::GAME_GRAB) == 0) {
        if (_game.triggerGrab()) {
            _sendAck(client, type);
        } else {
            _sendNack(client, type, "Cannot grab now");
        }
        return;
    }

    // --- Config ---
    if (strcmp(type, Cmd::GET_CONFIG) == 0) {
        JsonDocument resp;
        resp["type"] = Evt::CONFIG;
        // Embed config as nested object
        JsonDocument cfgDoc;
        deserializeJson(cfgDoc, machineConfig.toJson());
        resp["config"] = cfgDoc;
        String out;
        serializeJson(resp, out);
        client->text(out);
        return;
    }
    if (strcmp(type, Cmd::SET_CONFIG) == 0) {
        String cfgStr;
        serializeJson(doc["config"], cfgStr);
        if (machineConfig.applyFromJson(cfgStr.c_str())) {
            _motion.applyConfig();
            _sendAck(client, type);
        } else {
            _sendNack(client, type, "Invalid config");
        }
        return;
    }
    if (strcmp(type, Cmd::SAVE_CONFIG) == 0) {
        machineConfig.saveAll();
        _sendAck(client, type);
        return;
    }
    if (strcmp(type, Cmd::RESET_CONFIG) == 0) {
        machineConfig.resetToDefaults();
        _motion.applyConfig();
        _sendAck(client, type);
        return;
    }

    // --- Lighting ---
    if (strcmp(type, Cmd::LED_SCENE) == 0) {
        // Will be handled by LED controller callback
        _sendAck(client, type);
        return;
    }
    if (strcmp(type, Cmd::LED_BRIGHT) == 0) {
        int brightness = doc["brightness"] | 128;
        machineConfig.settings().led_brightness = constrain(brightness, 0, 255);
        _sendAck(client, type);
        return;
    }

    // --- Joystick calibration ---
    if (strcmp(type, Cmd::JOY_CAL_CENTER) == 0) {
        _joystick.calibrateCenter();
        _sendAck(client, type);
        return;
    }
    if (strcmp(type, Cmd::JOY_CAL_START) == 0) {
        _joystick.startRangeCalibration();
        _sendAck(client, type);
        return;
    }
    if (strcmp(type, Cmd::JOY_CAL_STOP) == 0) {
        _joystick.stopRangeCalibration();
        _sendAck(client, type);
        return;
    }

    // --- Motors ---
    if (strcmp(type, Cmd::ENABLE_MOTORS) == 0) {
        _motion.steppers().enableMotors();
        _sendAck(client, type);
        return;
    }
    if (strcmp(type, Cmd::DISABLE_MOTORS) == 0) {
        _motion.steppers().disableMotors();
        _sendAck(client, type);
        return;
    }

    // --- Maintenance ---
    if (strcmp(type, Cmd::MAINTENANCE) == 0) {
        bool enter = doc["enter"] | true;
        if (enter) _game.enterMaintenance();
        else _game.exitMaintenance();
        _sendAck(client, type);
        return;
    }

    // --- Set Position ---
    if (strcmp(type, Cmd::SET_POSITION) == 0) {
        float x = doc["x"] | -1.0f;
        float y = doc["y"] | -1.0f;
        float z = doc["z"] | -1.0f;
        if (x >= 0 && y >= 0) _motion.setPositionXY(x, y);
        if (z >= 0) _motion.setPositionZ(z);
        _motion.setPositionState(PositionState::HOMED);
        _sendAck(client, type);
        return;
    }

    // --- Logs ---
    if (strcmp(type, Cmd::GET_LOGS) == 0) {
        JsonDocument resp;
        resp["type"] = Evt::LOG;
        JsonArray logs = resp["entries"].to<JsonArray>();
        const LogEntry* buf = Logger::getBuffer();
        int count = Logger::getBufferCount();
        for (int i = 0; i < count && i < 32; i++) {
            JsonObject entry = logs.add<JsonObject>();
            entry["t"] = buf[i].timestamp;
            entry["l"] = static_cast<int>(buf[i].level);
            entry["m"] = buf[i].module;
            entry["msg"] = buf[i].message;
        }
        String out;
        serializeJson(resp, out);
        client->text(out);
        return;
    }

    // Unknown command
    _sendNack(client, type, "Unknown command");
}

void WebSocketServer::sendTelemetry() {
    if (!_ws.availableForWriteAll()) {
        return; // Skip non-critical telemetry to avoid queue starvation (prevents ping timeouts)
    }

    JsonDocument doc;
    doc["type"] = Evt::TELEMETRY;
    doc["state"] = _game.getStateString();
    doc["x"] = _motion.getX();
    doc["y"] = _motion.getY();
    doc["z"] = _motion.getZ();
    doc["homed"] = (_motion.getPositionState() == PositionState::HOMED);
    doc["moving"] = _motion.isMoving();

    // Claw
    doc["claw"] = static_cast<int>(_claw.getState());
    doc["claw_angle"] = _claw.getCurrentAngle();

    // Joystick
    const auto& joy = _joystick.getCalibrated();
    doc["joy_x"] = joy.x;
    doc["joy_y"] = joy.y;
    doc["joy_z"] = joy.z;
    doc["joy_btn"] = joy.button;

    // Sensors
    doc["sensors"] = _sensors.getSensorBitmask();
    doc["estop"] = _sensors.isEStopTriggered();
    doc["prize_beam"] = _sensors.prizeSensor().isBeamBroken();

    // Game
    doc["credits"] = _game.getCredits();
    doc["game_time"] = _game.getGameTimeRemainingMs();
    doc["games_played"] = _game.getStats().gamesPlayed;
    doc["games_won"] = _game.getStats().gamesWon;

    // Safety
    doc["safety"] = static_cast<int>(_safety.getState());
    doc["motors_on"] = _motion.steppers().areMotorsEnabled();

    doc["uptime"] = millis();

    String out;
    serializeJson(doc, out);
    _broadcast(out);
}

void WebSocketServer::sendStateChange(MachineState oldState, MachineState newState) {
    JsonDocument doc;
    doc["type"] = Evt::STATE_CHANGE;
    doc["from"] = _game.getStateString();
    doc["to"] = _game.getStateString();
    String out;
    serializeJson(doc, out);
    _broadcast(out);
}

void WebSocketServer::sendError(ErrorCode code) {
    JsonDocument doc;
    doc["type"] = Evt::ERROR;
    doc["code"] = static_cast<int>(code);
    doc["message"] = errorMessage(code);
    String out;
    serializeJson(doc, out);
    _broadcast(out);
}

void WebSocketServer::sendGameEvent(const char* event, const char* data) {
    JsonDocument doc;
    doc["type"] = Evt::GAME_EVENT;
    doc["event"] = event;
    doc["data"] = data;
    String out;
    serializeJson(doc, out);
    _broadcast(out);
}

void WebSocketServer::sendPrizeDetected() {
    JsonDocument doc;
    doc["type"] = Evt::PRIZE;
    doc["detected"] = true;
    String out;
    serializeJson(doc, out);
    _broadcast(out);
}

void WebSocketServer::sendEvent(const char* type, JsonDocument& doc) {
    doc["type"] = type;
    String out;
    serializeJson(doc, out);
    _broadcast(out);
}

void WebSocketServer::_sendAck(AsyncWebSocketClient* client, const char* cmdType) {
    JsonDocument doc;
    doc["type"] = Evt::ACK;
    doc["cmd"] = cmdType;
    String out;
    serializeJson(doc, out);
    client->text(out);
}

void WebSocketServer::_sendNack(AsyncWebSocketClient* client, const char* cmdType,
                                  const char* reason) {
    JsonDocument doc;
    doc["type"] = Evt::NACK;
    doc["cmd"] = cmdType;
    doc["reason"] = reason;
    String out;
    serializeJson(doc, out);
    client->text(out);
}

void WebSocketServer::_broadcast(const String& message) {
    _ws.textAll(message);
}
