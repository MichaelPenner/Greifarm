// =============================================================
// Main — Claw Machine ESP32 Firmware Entry Point
// =============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

#include "config/machine_config.h"
#include "config/pin_config.h"
#include "config/config_manager.h"
#include "motion/motion_planner.h"
#include "motion/homing.h"
#include "joystick/joystick_reader.h"
#include "claw/claw_controller.h"
#include "sensors/sensor_manager.h"
#include "safety/safety_controller.h"
#include "game/game_state_machine.h"
#include "communication/websocket_server.h"
#include "lighting/led_controller.h"
#include "utils/logger.h"
#include "utils/timer.h"

// =============================================================
// Global instances
// =============================================================

static MotionPlanner      motion;
static SensorManager      sensors;
static HomingController   homing(motion, sensors);
static ClawController     claw;
static SafetyController   safety(motion, sensors, claw);
static JoystickReader     joystick;
static GameStateMachine   game(motion, homing, claw, sensors, safety);
static LedController      leds;
static WebSocketServer*   wsServer = nullptr;

// Timers
static NonBlockingTimer   joystickTimer(JOY_READ_INTERVAL_MS);

// =============================================================
// WiFi Setup
// =============================================================

void setupWiFi() {
    const auto& cfg = machineConfig.settings();

    if (cfg.wifi_mode == 1 && strlen(cfg.wifi_ssid) > 0) {
        // Station mode — connect to existing network
        LOG_I("WIFI", "Connecting to %s...", cfg.wifi_ssid);
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(DEFAULT_HOSTNAME);
        WiFi.begin(cfg.wifi_ssid, cfg.wifi_password);

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 40) {
            delay(250);
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            LOG_I("WIFI", "Connected! IP: %s", WiFi.localIP().toString().c_str());
        } else {
            LOG_W("WIFI", "STA connection failed, falling back to AP mode");
            WiFi.mode(WIFI_AP);
            WiFi.softAP(DEFAULT_AP_SSID, DEFAULT_AP_PASSWORD);
            LOG_I("WIFI", "AP started: %s, IP: %s",
                  DEFAULT_AP_SSID, WiFi.softAPIP().toString().c_str());
        }
    } else {
        // Access Point mode
        WiFi.mode(WIFI_AP);
        WiFi.softAP(DEFAULT_AP_SSID, DEFAULT_AP_PASSWORD);
        LOG_I("WIFI", "AP mode: %s, IP: %s",
              DEFAULT_AP_SSID, WiFi.softAPIP().toString().c_str());
    }
}

// =============================================================
// State change callback — links game state to LED scenes
// =============================================================

void onStateChange(MachineState oldState, MachineState newState) {
    // Update LED scene based on machine state
    switch (newState) {
        case MachineState::BOOT:
        case MachineState::INITIALIZING:
            leds.setScene(LedScene::BOOT); break;
        case MachineState::HOMING:
            leds.setScene(LedScene::BOOT); break;
        case MachineState::IDLE:
            leds.setScene(LedScene::IDLE); break;
        case MachineState::READY:
            leds.setScene(LedScene::READY); break;
        case MachineState::PLAYING:
            leds.setScene(LedScene::GAME_START); break;
        case MachineState::CLAW_DESCENDING:
        case MachineState::CLAW_GRABBING:
            leds.setScene(LedScene::CLAW_CLOSE); break;
        case MachineState::CLAW_RETRACTING:
        case MachineState::RETURNING_TO_DROP:
        case MachineState::RELEASING:
            leds.setScene(LedScene::PLAYING); break;
        case MachineState::CHECKING_PRIZE:
            leds.setScene(LedScene::PRIZE_DETECTED); break;
        case MachineState::WIN:
            leds.setScene(LedScene::WIN); break;
        case MachineState::GAME_OVER:
            leds.setScene(LedScene::LOSS); break;
        case MachineState::ERROR_STATE:
            leds.setScene(LedScene::ERROR_SCENE); break;
        case MachineState::EMERGENCY_STOP:
            leds.setScene(LedScene::EMERGENCY_STOP); break;
        case MachineState::MAINTENANCE:
        case MachineState::CALIBRATION:
            leds.setScene(LedScene::MAINTENANCE); break;
    }

    // Broadcast state change via WebSocket
    if (wsServer) {
        wsServer->sendStateChange(oldState, newState);
    }
}

void onGameEvent(const char* event, const char* data) {
    if (wsServer) {
        wsServer->sendGameEvent(event, data);
    }
}

// =============================================================
// Setup
// =============================================================

void setup() {
    Serial.begin(115200);
    delay(SAFE_BOOT_DELAY_MS); // Wait for power to stabilize

    LOG_I("MAIN", "=================================");
    LOG_I("MAIN", "  Claw Machine — Firmware v1.0");
    LOG_I("MAIN", "=================================");

    // 1. Load configuration from NVS
    machineConfig.begin();

    // 2. Initialize WiFi
    setupWiFi();

    // 3. Initialize subsystems
    sensors.begin();
    motion.begin();
    claw.begin();
    safety.begin();
    joystick.begin();
    leds.begin();

    // 4. Register callbacks
    game.onStateChange(onStateChange);
    game.onGameEvent(onGameEvent);

    // 5. Start WebSocket server
    wsServer = new WebSocketServer(game, motion, homing, claw, sensors, safety, joystick);
    wsServer->begin();

    // 6. Start game engine (handles boot → init → homing → idle)
    game.begin();

    // 7. Start timers
    joystickTimer.start();

    // 8. Start OTA
    ArduinoOTA.setHostname(DEFAULT_HOSTNAME);
    ArduinoOTA.begin();

    LOG_I("MAIN", "Setup complete — entering main loop");
}

// =============================================================
// Main Loop
// =============================================================

void loop() {
    // 1. Update sensors (highest priority — includes E-stop)
    sensors.update();

    // 2. Safety check (BEFORE motion)
    safety.update();

    // 3. Read joystick
    if (joystickTimer.elapsed()) {
        joystick.update();

        // Feed joystick to motion during PLAYING state
        if (game.getState() == MachineState::PLAYING && safety.isMovementAllowed()) {
            const auto& joy = joystick.getCalibrated();
            motion.setJoystickSpeed(joy.x, joy.y, joy.z);

            // Check grab button
            if (joy.buttonPressed) {
                game.triggerGrab();
            }
        }

        // In maintenance mode, also allow joystick control
        if (game.getState() == MachineState::MAINTENANCE && safety.isMovementAllowed()) {
            const auto& joy = joystick.getCalibrated();
            motion.setJoystickSpeed(joy.x, joy.y, joy.z);

            if (joy.buttonPressed) {
                // Toggle claw in maintenance
                if (claw.getState() == ClawState::OPEN) {
                    claw.close();
                } else {
                    claw.open();
                }
            }
        }
    }

    // 4. Update claw
    claw.update();

    // 5. Update motion (generates step pulses)
    motion.update();

    // 6. Update game state machine
    game.update();

    // 7. Update LED animations
    leds.update();

    // 8. Update WebSocket (telemetry, incoming commands)
    if (wsServer) {
        wsServer->update();
    }

    // 9. Prize detection event relay
    if (sensors.isPrizeDetected() && wsServer) {
        wsServer->sendPrizeDetected();
    }

    // 10. Handle OTA updates
    ArduinoOTA.handle();
}
