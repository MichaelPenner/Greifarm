#pragma once
// =============================================================
// Config Manager — Persistent Configuration via NVS
// =============================================================
// Stores and retrieves runtime configuration from ESP32 NVS
// (Non-Volatile Storage). This allows calibration values and
// settings to survive power cycles without reflashing.
// =============================================================

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include "machine_config.h"

// Runtime configuration structure — mutable copy of defaults
struct MachineSettings {
    // Motion
    float steps_per_mm;
    float x_max_mm;
    float x_max_speed;
    float x_acceleration;
    float y_max_mm;
    float y_max_speed;
    float y_acceleration;
    float z_max_mm;
    float z_max_speed;
    float z_acceleration;
    float z_steps_per_mm;
    bool invert_motor_a;
    bool invert_motor_b;
    bool invert_motor_z;

    // Homing
    float x_home_speed;
    float y_home_speed;
    float z_home_speed;
    float x_home_backoff;
    float y_home_backoff;
    float z_home_backoff;

    // Drop zone
    float drop_x;
    float drop_y;

    // Park position
    float park_x;
    float park_y;

    // Claw
    int claw_open_angle;
    int claw_close_angle;
    int claw_partial_angle;
    int claw_grip_delay_ms;
    int claw_move_speed_ms;

    // Joystick
    int joy_x_center;
    int joy_y_center;
    int joy_z_center;
    int joy_deadzone;
    int joy_min;
    int joy_max;
    float joy_sensitivity;
    float joy_expo;
    bool joy_invert_x;
    bool joy_invert_y;
    bool joy_invert_z;

    // Game
    int game_duration_s;
    int credits_per_game;
    bool free_play;
    int claw_strength;
    bool auto_home_on_boot;
    bool re_home_between_games;

    // Lighting
    int led_count;
    int led_brightness;

    // WiFi
    int wifi_mode;
    char wifi_ssid[33];
    char wifi_password[65];

    // Safety
    bool z_always_energized;
};

class ConfigManager {
public:
    ConfigManager();

    // Initialize NVS and load settings
    bool begin();

    // Load all settings from NVS (or use defaults if not stored)
    void loadDefaults();

    // Save current settings to NVS
    bool saveAll();

    // Save a specific section
    bool saveMotion();
    bool saveJoystick();
    bool saveClaw();
    bool saveGame();
    bool saveLighting();
    bool saveWifi();

    // Reset all settings to compile-time defaults
    void resetToDefaults();

    // Access settings
    MachineSettings& settings() { return _settings; }
    const MachineSettings& settings() const { return _settings; }

    // Apply settings from JSON string (from WebSocket/API)
    bool applyFromJson(const char* json);

    // Export settings as JSON string
    String toJson() const;

private:
    MachineSettings _settings;
    bool _initialized;

    void _loadFromNVS();
    void _saveToNVS();
};

// Global config instance
extern ConfigManager machineConfig;

#endif // CONFIG_MANAGER_H
