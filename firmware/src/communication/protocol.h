#pragma once
// =============================================================
// Protocol — Message Type Definitions for ESP ↔ Backend
// =============================================================

#ifndef PROTOCOL_H
#define PROTOCOL_H

// =============================================================
// Command types (Backend → ESP)
// =============================================================
namespace Cmd {
    constexpr const char* HEARTBEAT     = "heartbeat";
    constexpr const char* MOVE          = "move";        // Joystick-style continuous
    constexpr const char* MOVE_TO       = "move_to";     // Absolute position
    constexpr const char* MOVE_REL      = "move_rel";    // Relative position
    constexpr const char* HOME          = "home";        // Home all or specific axis
    constexpr const char* STOP          = "stop";        // Controlled stop
    constexpr const char* ESTOP         = "estop";       // Emergency stop
    constexpr const char* RESET         = "reset";       // Reset after error/estop
    constexpr const char* CLAW_OPEN     = "claw_open";
    constexpr const char* CLAW_CLOSE    = "claw_close";
    constexpr const char* CLAW_PARTIAL  = "claw_partial";
    constexpr const char* CLAW_ANGLE    = "claw_angle";  // Set specific angle
    constexpr const char* GAME_START    = "game_start";
    constexpr const char* GAME_STOP     = "game_stop";
    constexpr const char* GAME_GRAB     = "game_grab";   // Player presses grab
    constexpr const char* SET_CONFIG    = "set_config";   // Apply config JSON
    constexpr const char* GET_CONFIG    = "get_config";   // Request config
    constexpr const char* SAVE_CONFIG   = "save_config";  // Save to NVS
    constexpr const char* RESET_CONFIG  = "reset_config"; // Reset to defaults
    constexpr const char* LED_SCENE     = "led_scene";    // Set lighting scene
    constexpr const char* LED_BRIGHT    = "led_bright";   // Set brightness
    constexpr const char* JOY_CAL_CENTER= "joy_cal_center"; // Calibrate joystick center
    constexpr const char* JOY_CAL_START = "joy_cal_range_start";
    constexpr const char* JOY_CAL_STOP  = "joy_cal_range_stop";
    constexpr const char* GET_LOGS      = "get_logs";     // Request log buffer
    constexpr const char* ENABLE_MOTORS = "enable_motors";
    constexpr const char* DISABLE_MOTORS= "disable_motors";
    constexpr const char* SET_POSITION  = "set_position"; // Set current position (calibration)
    constexpr const char* MAINTENANCE   = "maintenance";  // Enter/exit maintenance mode
}

// =============================================================
// Event types (ESP → Backend)
// =============================================================
namespace Evt {
    constexpr const char* TELEMETRY     = "telemetry";    // Periodic state update
    constexpr const char* HEARTBEAT_ACK = "heartbeat_ack";
    constexpr const char* STATE_CHANGE  = "state_change"; // Machine state transition
    constexpr const char* ERROR         = "error";        // Error occurred
    constexpr const char* PRIZE         = "prize";        // Prize detected
    constexpr const char* GAME_EVENT    = "game_event";   // Game state change
    constexpr const char* HOMING_STATUS = "homing_status";// Homing progress
    constexpr const char* CONFIG        = "config";       // Config response
    constexpr const char* LOG           = "log";          // Log entries
    constexpr const char* MOVE_COMPLETE = "move_complete"; // Planned move finished
    constexpr const char* ACK           = "ack";          // Command acknowledged
    constexpr const char* NACK          = "nack";         // Command rejected
}

// =============================================================
// Machine states (string representation for JSON)
// =============================================================
namespace StateStr {
    constexpr const char* BOOT              = "BOOT";
    constexpr const char* INITIALIZING      = "INITIALIZING";
    constexpr const char* HOMING            = "HOMING";
    constexpr const char* IDLE              = "IDLE";
    constexpr const char* READY             = "READY";
    constexpr const char* PLAYING           = "PLAYING";
    constexpr const char* CLAW_DESCENDING   = "CLAW_DESCENDING";
    constexpr const char* CLAW_GRABBING     = "CLAW_GRABBING";
    constexpr const char* CLAW_RETRACTING   = "CLAW_RETRACTING";
    constexpr const char* RETURNING         = "RETURNING";
    constexpr const char* RELEASING         = "RELEASING";
    constexpr const char* CHECKING_PRIZE    = "CHECKING_PRIZE";
    constexpr const char* WIN               = "WIN";
    constexpr const char* GAME_OVER         = "GAME_OVER";
    constexpr const char* ERROR_STATE       = "ERROR";
    constexpr const char* EMERGENCY_STOP    = "EMERGENCY_STOP";
    constexpr const char* MAINTENANCE       = "MAINTENANCE";
    constexpr const char* CALIBRATION       = "CALIBRATION";
}

// =============================================================
// Claw states (string representation)
// =============================================================
namespace ClawStr {
    constexpr const char* UNKNOWN   = "UNKNOWN";
    constexpr const char* OPEN      = "OPEN";
    constexpr const char* CLOSED    = "CLOSED";
    constexpr const char* PARTIAL   = "PARTIAL";
    constexpr const char* MOVING    = "MOVING";
}

#endif // PROTOCOL_H
