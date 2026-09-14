#pragma once
// =============================================================
// Error Codes — Structured Error System
// =============================================================

#ifndef ERROR_CODES_H
#define ERROR_CODES_H

// Error severity levels
enum class ErrorSeverity : uint8_t {
    INFO    = 0,
    WARNING = 1,
    ERROR   = 2,
    FATAL   = 3
};

// Structured error codes
enum class ErrorCode : uint16_t {
    NONE                    = 0,

    // Motion errors (E001–E099)
    NOT_HOMED               = 1,
    LIMIT_TRIGGERED         = 2,
    MOTOR_TIMEOUT           = 3,
    POSITION_OUT_OF_BOUNDS  = 4,
    COREXY_TRANSFORM_ERROR  = 5,
    MOVE_QUEUE_FULL         = 6,
    HOMING_FAILED           = 7,
    MOTOR_STALL_DETECTED    = 8,

    // Communication errors (E100–E199)
    COMMUNICATION_TIMEOUT   = 100,
    WEBSOCKET_DISCONNECTED  = 101,
    INVALID_COMMAND         = 102,
    JSON_PARSE_ERROR        = 103,
    HEARTBEAT_TIMEOUT       = 104,

    // Sensor errors (E200–E299)
    SENSOR_FAILURE          = 200,
    PRIZE_SENSOR_FAULT      = 201,
    LIMIT_SWITCH_FAULT      = 202,
    JOYSTICK_FAULT          = 203,
    ADC_READ_ERROR          = 204,

    // Claw errors (E300–E399)
    CLAW_TIMEOUT            = 300,
    SERVO_FAULT             = 301,
    CLAW_POSITION_ERROR     = 302,

    // Safety errors (E400–E499)
    EMERGENCY_STOP          = 400,
    WATCHDOG_TIMEOUT        = 401,
    POWER_FAULT             = 402,
    OVERTEMPERATURE          = 403,
    SAFETY_INTERLOCK        = 404,

    // Game errors (E500–E599)
    GAME_STATE_ERROR        = 500,
    NO_CREDITS              = 501,
    GAME_TIMEOUT            = 502,

    // Configuration errors (E600–E699)
    CONFIG_LOAD_ERROR       = 600,
    CONFIG_SAVE_ERROR       = 601,
    INVALID_CONFIG          = 602,

    // System errors (E900–E999)
    SYSTEM_ERROR            = 900,
    MEMORY_ERROR            = 901,
    INITIALIZATION_FAILED   = 902
};

// Error info structure
struct ErrorInfo {
    ErrorCode code;
    ErrorSeverity severity;
    const char* message;
    unsigned long timestamp;  // millis() when error occurred
    bool active;              // Currently active error
};

// Human-readable error messages
inline const char* errorMessage(ErrorCode code) {
    switch (code) {
        case ErrorCode::NONE:                   return "No error";
        case ErrorCode::NOT_HOMED:              return "Machine not homed — home axes before moving";
        case ErrorCode::LIMIT_TRIGGERED:        return "Limit switch triggered during movement";
        case ErrorCode::MOTOR_TIMEOUT:          return "Motor movement timed out";
        case ErrorCode::POSITION_OUT_OF_BOUNDS: return "Requested position outside travel limits";
        case ErrorCode::COREXY_TRANSFORM_ERROR: return "CoreXY coordinate transformation error";
        case ErrorCode::MOVE_QUEUE_FULL:        return "Motion queue full — slow down commands";
        case ErrorCode::HOMING_FAILED:          return "Homing sequence failed — check limit switches";
        case ErrorCode::MOTOR_STALL_DETECTED:   return "Motor stall detected — check belt tension";
        case ErrorCode::COMMUNICATION_TIMEOUT:  return "Communication timeout — backend disconnected";
        case ErrorCode::WEBSOCKET_DISCONNECTED: return "WebSocket connection lost";
        case ErrorCode::INVALID_COMMAND:        return "Invalid command received";
        case ErrorCode::JSON_PARSE_ERROR:       return "Failed to parse JSON message";
        case ErrorCode::HEARTBEAT_TIMEOUT:      return "Heartbeat timeout — no response from client";
        case ErrorCode::SENSOR_FAILURE:         return "Sensor failure detected";
        case ErrorCode::PRIZE_SENSOR_FAULT:     return "Prize detection sensor fault";
        case ErrorCode::LIMIT_SWITCH_FAULT:     return "Limit switch fault detected";
        case ErrorCode::JOYSTICK_FAULT:         return "Joystick reading out of expected range";
        case ErrorCode::ADC_READ_ERROR:         return "ADC read error";
        case ErrorCode::CLAW_TIMEOUT:           return "Claw operation timed out";
        case ErrorCode::SERVO_FAULT:            return "Servo fault detected";
        case ErrorCode::CLAW_POSITION_ERROR:    return "Claw position error";
        case ErrorCode::EMERGENCY_STOP:         return "EMERGENCY STOP ACTIVATED";
        case ErrorCode::WATCHDOG_TIMEOUT:       return "Motion watchdog timeout — movement aborted";
        case ErrorCode::POWER_FAULT:            return "Power supply fault detected";
        case ErrorCode::OVERTEMPERATURE:         return "Overtemperature warning";
        case ErrorCode::SAFETY_INTERLOCK:       return "Safety interlock triggered";
        case ErrorCode::GAME_STATE_ERROR:       return "Invalid game state transition";
        case ErrorCode::NO_CREDITS:             return "No credits — insert coin or enable free play";
        case ErrorCode::GAME_TIMEOUT:           return "Game timer expired";
        case ErrorCode::CONFIG_LOAD_ERROR:      return "Failed to load configuration";
        case ErrorCode::CONFIG_SAVE_ERROR:      return "Failed to save configuration";
        case ErrorCode::INVALID_CONFIG:         return "Invalid configuration value";
        case ErrorCode::SYSTEM_ERROR:           return "System error";
        case ErrorCode::MEMORY_ERROR:           return "Memory allocation failed";
        case ErrorCode::INITIALIZATION_FAILED:  return "System initialization failed";
        default:                                return "Unknown error";
    }
}

inline const char* severityString(ErrorSeverity sev) {
    switch (sev) {
        case ErrorSeverity::INFO:    return "INFO";
        case ErrorSeverity::WARNING: return "WARN";
        case ErrorSeverity::ERROR:   return "ERROR";
        case ErrorSeverity::FATAL:   return "FATAL";
        default:                     return "???";
    }
}

#endif // ERROR_CODES_H
