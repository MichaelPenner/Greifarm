#pragma once
// =============================================================
// WebSocket Server + Command Handler + Telemetry
// =============================================================

#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "../game/game_state_machine.h"
#include "../motion/motion_planner.h"
#include "../motion/homing.h"
#include "../claw/claw_controller.h"
#include "../sensors/sensor_manager.h"
#include "../safety/safety_controller.h"
#include "../joystick/joystick_reader.h"
#include "../config/config_manager.h"
#include "../utils/logger.h"
#include "../utils/timer.h"
#include "protocol.h"

class WebSocketServer {
public:
    WebSocketServer(GameStateMachine& game, MotionPlanner& motion,
                    HomingController& homing, ClawController& claw,
                    SensorManager& sensors, SafetyController& safety,
                    JoystickReader& joystick);

    void begin();
    void update();

    // Send events to connected clients
    void sendEvent(const char* type, JsonDocument& doc);
    void sendTelemetry();
    void sendStateChange(MachineState oldState, MachineState newState);
    void sendError(ErrorCode code);
    void sendGameEvent(const char* event, const char* data);
    void sendPrizeDetected();

    bool hasClients() const { return _ws.count() > 0; }

private:
    AsyncWebServer _server;
    AsyncWebSocket _ws;

    GameStateMachine& _game;
    MotionPlanner& _motion;
    HomingController& _homing;
    ClawController& _claw;
    SensorManager& _sensors;
    SafetyController& _safety;
    JoystickReader& _joystick;

    NonBlockingTimer _telemetryTimer;

    void _onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                    AwsEventType type, void* arg, uint8_t* data, size_t len);
    void _handleMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len);
    void _handleCommand(AsyncWebSocketClient* client, JsonDocument& doc);

    void _sendAck(AsyncWebSocketClient* client, const char* cmdType);
    void _sendNack(AsyncWebSocketClient* client, const char* cmdType, const char* reason);
    void _broadcast(const String& message);
};

#endif // WEBSOCKET_SERVER_H
