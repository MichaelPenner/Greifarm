#pragma once
// =============================================================
// Machine Configuration — Claw Machine
// =============================================================
// All tunable machine parameters live here.
// These are DEFAULT values — runtime overrides are stored in NVS.
// Change these defaults to match your hardware, then reflash.
// =============================================================

#ifndef MACHINE_CONFIG_H
#define MACHINE_CONFIG_H

// =============================================================
// Motion — CoreXY
// =============================================================

// Stepper motor specs (Two Trees Sapphire S NEMA 17)
#define DEFAULT_STEPS_PER_REV       200     // 1.8° stepper
#define DEFAULT_MICROSTEPPING       16      // A4988 set to 1/16

// GT2 belt + pulley (Sapphire S)
#define DEFAULT_PULLEY_TEETH        20      // 20-tooth GT2 pulley
#define DEFAULT_BELT_PITCH_MM       2.0f    // GT2 = 2mm pitch

// Calculated: steps_per_mm = (steps_per_rev * microstepping) / (teeth * pitch)
// = (200 * 16) / (20 * 2) = 80 steps/mm
#define DEFAULT_STEPS_PER_MM        80.0f

// CoreXY motor orientation
#define DEFAULT_INVERT_MOTOR_A      false
#define DEFAULT_INVERT_MOTOR_B      false
#define DEFAULT_INVERT_MOTOR_Z      false

// X axis
#define DEFAULT_X_MAX_MM            200.0f  // Sapphire S build volume
#define DEFAULT_X_MAX_SPEED         100.0f  // mm/s
#define DEFAULT_X_ACCELERATION      300.0f  // mm/s²
#define DEFAULT_X_HOME_SPEED        30.0f   // mm/s — slow for homing
#define DEFAULT_X_HOME_BACKOFF_MM   5.0f    // Back off after hitting switch

// Y axis
#define DEFAULT_Y_MAX_MM            200.0f
#define DEFAULT_Y_MAX_SPEED         100.0f
#define DEFAULT_Y_ACCELERATION      300.0f
#define DEFAULT_Y_HOME_SPEED        30.0f
#define DEFAULT_Y_HOME_BACKOFF_MM   5.0f

// Z axis (claw lift — spool + cable)
#define DEFAULT_Z_MAX_MM            180.0f  // Total Z travel
#define DEFAULT_Z_MAX_SPEED         50.0f   // mm/s — slower for safety
#define DEFAULT_Z_ACCELERATION      100.0f  // mm/s²
#define DEFAULT_Z_HOME_SPEED        20.0f   // mm/s
#define DEFAULT_Z_HOME_BACKOFF_MM   3.0f
#define DEFAULT_Z_STEPS_PER_MM      80.0f   // TBD — depends on spool diameter

// Drop zone position (where claw opens to release prize)
#define DEFAULT_DROP_X_MM           100.0f  // Center-ish
#define DEFAULT_DROP_Y_MM           180.0f  // Near the prize chute

// Park position (idle position)
#define DEFAULT_PARK_X_MM           100.0f
#define DEFAULT_PARK_Y_MM           100.0f
#define DEFAULT_PARK_Z_MM           0.0f    // Z fully retracted (top)

// =============================================================
// Stepper Timing
// =============================================================

#define STEP_PULSE_WIDTH_US         5       // Minimum step pulse width (µs)
#define STEP_INTERVAL_MIN_US        50      // Minimum step interval (µs) = max speed

// =============================================================
// Claw Servo
// =============================================================

#define DEFAULT_CLAW_OPEN_ANGLE     30      // Servo degrees — fully open
#define DEFAULT_CLAW_CLOSE_ANGLE    130     // Servo degrees — fully closed
#define DEFAULT_CLAW_PARTIAL_ANGLE  80      // Servo degrees — partial grip
#define DEFAULT_CLAW_GRIP_DELAY_MS  500     // Time to hold grip before retracting
#define DEFAULT_CLAW_MOVE_SPEED_MS  300     // Time for servo to reach position

// Servo PWM
#define SERVO_MIN_PULSE_US          544     // 0° pulse width
#define SERVO_MAX_PULSE_US          2400    // 180° pulse width
#define SERVO_PWM_FREQ_HZ           50      // Standard servo frequency

// =============================================================
// Joystick (JH-D400X-R4)
// =============================================================

// ADC range (ESP32 12-bit ADC, 0–4095)
#define DEFAULT_JOY_X_CENTER        2048    // Calibrate to actual center
#define DEFAULT_JOY_Y_CENTER        2048
#define DEFAULT_JOY_Z_CENTER        2048
#define DEFAULT_JOY_DEADZONE        150     // ADC units — ignore small movements
#define DEFAULT_JOY_MIN             0       // ADC minimum
#define DEFAULT_JOY_MAX             4095    // ADC maximum

// Joystick sensitivity (0.0 = off, 1.0 = maximum)
#define DEFAULT_JOY_SENSITIVITY     0.7f

// Exponential response curve exponent (1.0 = linear, 2.0 = quadratic)
#define DEFAULT_JOY_EXPO            1.5f

// Direction inversion
#define DEFAULT_JOY_INVERT_X        false
#define DEFAULT_JOY_INVERT_Y        false
#define DEFAULT_JOY_INVERT_Z        false

// Joystick read interval
#define JOY_READ_INTERVAL_MS        20      // 50 Hz polling
#define JOY_ADC_SAMPLES             4       // Average N samples per read

// =============================================================
// Limit Switches
// =============================================================

// Debounce time
#define SWITCH_DEBOUNCE_MS          5       // Milliseconds
#define SWITCH_ACTIVE_LOW           true    // NC switches: LOW = triggered

// =============================================================
// Prize Detection
// =============================================================

#define PRIZE_DEBOUNCE_MS           100     // Ignore brief flickers
#define PRIZE_MIN_BLOCK_MS          50      // Min beam-break duration for valid prize
#define PRIZE_COOLDOWN_MS           3000    // Ignore sensor after detection (prevent double-count)

// =============================================================
// Emergency Stop
// =============================================================

#define ESTOP_DEBOUNCE_MS           10      // Fast response
#define ESTOP_ACTIVE_LOW            true    // NC button: LOW = stopped

// =============================================================
// Game Settings
// =============================================================

#define DEFAULT_GAME_DURATION_S     30      // Seconds per game
#define DEFAULT_CREDITS_PER_GAME    1       // Credits consumed per game
#define DEFAULT_FREE_PLAY           true    // Free play mode on by default
#define DEFAULT_CLAW_STRENGTH       100     // Percentage (100 = full close angle)
#define DEFAULT_AUTO_HOME_ON_BOOT   true    // Home axes on startup
#define DEFAULT_RE_HOME_BETWEEN     true    // Re-home between games for accuracy

// Prize check timeout — how long to wait for prize sensor after release
#define PRIZE_CHECK_TIMEOUT_MS      3000

// =============================================================
// Communication
// =============================================================

#define WEBSOCKET_PORT              81      // ESP32 WebSocket server port
#define TELEMETRY_INTERVAL_MS       100     // Send telemetry every 100ms (10 Hz)
#define HEARTBEAT_INTERVAL_MS       2000    // Heartbeat ping interval
#define HEARTBEAT_TIMEOUT_MS        5000    // Disconnect if no pong received
#define MAX_WEBSOCKET_CLIENTS       3       // Max simultaneous WebSocket connections

// =============================================================
// WiFi
// =============================================================

// Station mode (connect to existing network)
#define DEFAULT_WIFI_MODE           1       // 0 = AP, 1 = STA
#define DEFAULT_WIFI_SSID           ""      // Set via config or NVS
#define DEFAULT_WIFI_PASSWORD       ""      // Set via config or NVS

// Access Point mode (fallback)
#define DEFAULT_AP_SSID             "ClawMachine"
#define DEFAULT_AP_PASSWORD         "claw1234"

// Hostname
#define DEFAULT_HOSTNAME            "clawmachine"

// =============================================================
// WS2812B Lighting
// =============================================================

#define DEFAULT_LED_COUNT           120     // Total LEDs in strip
#define DEFAULT_LED_BRIGHTNESS      128     // 0–255 (50% default to save power)
#define DEFAULT_LED_MAX_POWER_MW    10000   // Max power budget in milliwatts
#define LED_COLOR_ORDER             GRB     // WS2812B standard
#define LED_CHIPSET                 WS2812B

// Animation timing
#define LED_UPDATE_INTERVAL_MS      20      // 50 FPS LED refresh

// =============================================================
// Safety
// =============================================================

#define MOTION_WATCHDOG_MS          10000   // Max time for any single move
#define COMM_TIMEOUT_MS             10000   // Disable joystick if no heartbeat
#define MOTOR_IDLE_TIMEOUT_MS       60000   // Disable motors after 60s idle (except Z)
#define Z_ALWAYS_ENERGIZED          true    // Keep Z motor energized to prevent drop
#define SAFE_BOOT_DELAY_MS          2000    // Wait before initializing motors on boot

// =============================================================
// Logging
// =============================================================

#define LOG_LEVEL_DEFAULT           2       // 0=NONE, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG
#define LOG_BUFFER_SIZE             64      // Number of log entries to keep in memory

// =============================================================
// Simulation
// =============================================================

#define SIMULATION_MODE             false   // Set true for development without hardware

#endif // MACHINE_CONFIG_H
