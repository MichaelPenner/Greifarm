// =============================================================
// Config Manager — Implementation
// =============================================================

#include "config_manager.h"

#ifndef NATIVE_BUILD
#include <Preferences.h>
#include <ArduinoJson.h>
#endif

// Global instance
ConfigManager machineConfig;

ConfigManager::ConfigManager() : _initialized(false) {
    loadDefaults();
}

void ConfigManager::loadDefaults() {
    // Motion
    _settings.steps_per_mm      = DEFAULT_STEPS_PER_MM;
    _settings.x_max_mm          = DEFAULT_X_MAX_MM;
    _settings.x_max_speed       = DEFAULT_X_MAX_SPEED;
    _settings.x_acceleration    = DEFAULT_X_ACCELERATION;
    _settings.y_max_mm          = DEFAULT_Y_MAX_MM;
    _settings.y_max_speed       = DEFAULT_Y_MAX_SPEED;
    _settings.y_acceleration    = DEFAULT_Y_ACCELERATION;
    _settings.z_max_mm          = DEFAULT_Z_MAX_MM;
    _settings.z_max_speed       = DEFAULT_Z_MAX_SPEED;
    _settings.z_acceleration    = DEFAULT_Z_ACCELERATION;
    _settings.z_steps_per_mm    = DEFAULT_Z_STEPS_PER_MM;
    _settings.invert_motor_a    = DEFAULT_INVERT_MOTOR_A;
    _settings.invert_motor_b    = DEFAULT_INVERT_MOTOR_B;
    _settings.invert_motor_z    = DEFAULT_INVERT_MOTOR_Z;

    // Homing
    _settings.x_home_speed      = DEFAULT_X_HOME_SPEED;
    _settings.y_home_speed      = DEFAULT_Y_HOME_SPEED;
    _settings.z_home_speed      = DEFAULT_Z_HOME_SPEED;
    _settings.x_home_backoff    = DEFAULT_X_HOME_BACKOFF_MM;
    _settings.y_home_backoff    = DEFAULT_Y_HOME_BACKOFF_MM;
    _settings.z_home_backoff    = DEFAULT_Z_HOME_BACKOFF_MM;

    // Drop zone
    _settings.drop_x            = DEFAULT_DROP_X_MM;
    _settings.drop_y            = DEFAULT_DROP_Y_MM;

    // Park
    _settings.park_x            = DEFAULT_PARK_X_MM;
    _settings.park_y            = DEFAULT_PARK_Y_MM;

    // Claw
    _settings.claw_open_angle   = DEFAULT_CLAW_OPEN_ANGLE;
    _settings.claw_close_angle  = DEFAULT_CLAW_CLOSE_ANGLE;
    _settings.claw_partial_angle= DEFAULT_CLAW_PARTIAL_ANGLE;
    _settings.claw_grip_delay_ms= DEFAULT_CLAW_GRIP_DELAY_MS;
    _settings.claw_move_speed_ms= DEFAULT_CLAW_MOVE_SPEED_MS;

    // Joystick
    _settings.joy_x_center      = DEFAULT_JOY_X_CENTER;
    _settings.joy_y_center       = DEFAULT_JOY_Y_CENTER;
    _settings.joy_z_center       = DEFAULT_JOY_Z_CENTER;
    _settings.joy_deadzone       = DEFAULT_JOY_DEADZONE;
    _settings.joy_min            = DEFAULT_JOY_MIN;
    _settings.joy_max            = DEFAULT_JOY_MAX;
    _settings.joy_sensitivity    = DEFAULT_JOY_SENSITIVITY;
    _settings.joy_expo           = DEFAULT_JOY_EXPO;
    _settings.joy_invert_x       = DEFAULT_JOY_INVERT_X;
    _settings.joy_invert_y       = DEFAULT_JOY_INVERT_Y;
    _settings.joy_invert_z       = DEFAULT_JOY_INVERT_Z;

    // Game
    _settings.game_duration_s    = DEFAULT_GAME_DURATION_S;
    _settings.credits_per_game   = DEFAULT_CREDITS_PER_GAME;
    _settings.free_play          = DEFAULT_FREE_PLAY;
    _settings.claw_strength      = DEFAULT_CLAW_STRENGTH;
    _settings.auto_home_on_boot  = DEFAULT_AUTO_HOME_ON_BOOT;
    _settings.re_home_between_games = DEFAULT_RE_HOME_BETWEEN;

    // Lighting
    _settings.led_count          = DEFAULT_LED_COUNT;
    _settings.led_brightness     = DEFAULT_LED_BRIGHTNESS;

    // WiFi
    _settings.wifi_mode          = DEFAULT_WIFI_MODE;
    strncpy(_settings.wifi_ssid, DEFAULT_WIFI_SSID, sizeof(_settings.wifi_ssid));
    strncpy(_settings.wifi_password, DEFAULT_WIFI_PASSWORD, sizeof(_settings.wifi_password));

    // Safety
    _settings.z_always_energized = Z_ALWAYS_ENERGIZED;
}

#ifndef NATIVE_BUILD

bool ConfigManager::begin() {
    _loadFromNVS();
    _initialized = true;
    return true;
}

void ConfigManager::_loadFromNVS() {
    Preferences prefs;
    if (!prefs.begin("claw_cfg", true)) { // read-only
        Serial.println("[CONFIG] No saved config found, using defaults");
        return;
    }

    // Motion
    _settings.steps_per_mm   = prefs.getFloat("spm", _settings.steps_per_mm);
    _settings.x_max_mm       = prefs.getFloat("x_max", _settings.x_max_mm);
    _settings.x_max_speed    = prefs.getFloat("x_spd", _settings.x_max_speed);
    _settings.x_acceleration = prefs.getFloat("x_acc", _settings.x_acceleration);
    _settings.y_max_mm       = prefs.getFloat("y_max", _settings.y_max_mm);
    _settings.y_max_speed    = prefs.getFloat("y_spd", _settings.y_max_speed);
    _settings.y_acceleration = prefs.getFloat("y_acc", _settings.y_acceleration);
    _settings.z_max_mm       = prefs.getFloat("z_max", _settings.z_max_mm);
    _settings.z_max_speed    = prefs.getFloat("z_spd", _settings.z_max_speed);
    _settings.z_acceleration = prefs.getFloat("z_acc", _settings.z_acceleration);
    _settings.z_steps_per_mm = prefs.getFloat("z_spm", _settings.z_steps_per_mm);
    _settings.invert_motor_a = prefs.getBool("inv_a", _settings.invert_motor_a);
    _settings.invert_motor_b = prefs.getBool("inv_b", _settings.invert_motor_b);
    _settings.invert_motor_z = prefs.getBool("inv_z", _settings.invert_motor_z);

    // Homing
    _settings.x_home_speed   = prefs.getFloat("xh_spd", _settings.x_home_speed);
    _settings.y_home_speed   = prefs.getFloat("yh_spd", _settings.y_home_speed);
    _settings.z_home_speed   = prefs.getFloat("zh_spd", _settings.z_home_speed);
    _settings.x_home_backoff = prefs.getFloat("xh_bo", _settings.x_home_backoff);
    _settings.y_home_backoff = prefs.getFloat("yh_bo", _settings.y_home_backoff);
    _settings.z_home_backoff = prefs.getFloat("zh_bo", _settings.z_home_backoff);

    // Drop / Park
    _settings.drop_x         = prefs.getFloat("drp_x", _settings.drop_x);
    _settings.drop_y         = prefs.getFloat("drp_y", _settings.drop_y);
    _settings.park_x         = prefs.getFloat("prk_x", _settings.park_x);
    _settings.park_y         = prefs.getFloat("prk_y", _settings.park_y);

    // Claw
    _settings.claw_open_angle    = prefs.getInt("c_open", _settings.claw_open_angle);
    _settings.claw_close_angle   = prefs.getInt("c_close", _settings.claw_close_angle);
    _settings.claw_partial_angle = prefs.getInt("c_part", _settings.claw_partial_angle);
    _settings.claw_grip_delay_ms = prefs.getInt("c_grip", _settings.claw_grip_delay_ms);

    // Joystick
    _settings.joy_x_center   = prefs.getInt("jx_c", _settings.joy_x_center);
    _settings.joy_y_center   = prefs.getInt("jy_c", _settings.joy_y_center);
    _settings.joy_z_center   = prefs.getInt("jz_c", _settings.joy_z_center);
    _settings.joy_deadzone   = prefs.getInt("j_dz", _settings.joy_deadzone);
    _settings.joy_sensitivity= prefs.getFloat("j_sens", _settings.joy_sensitivity);
    _settings.joy_expo       = prefs.getFloat("j_expo", _settings.joy_expo);
    _settings.joy_invert_x   = prefs.getBool("jinv_x", _settings.joy_invert_x);
    _settings.joy_invert_y   = prefs.getBool("jinv_y", _settings.joy_invert_y);
    _settings.joy_invert_z   = prefs.getBool("jinv_z", _settings.joy_invert_z);

    // Game
    _settings.game_duration_s    = prefs.getInt("g_dur", _settings.game_duration_s);
    _settings.credits_per_game   = prefs.getInt("g_cred", _settings.credits_per_game);
    _settings.free_play          = prefs.getBool("g_free", _settings.free_play);
    _settings.claw_strength      = prefs.getInt("g_str", _settings.claw_strength);
    _settings.auto_home_on_boot  = prefs.getBool("g_ahome", _settings.auto_home_on_boot);
    _settings.re_home_between_games = prefs.getBool("g_rehome", _settings.re_home_between_games);

    // Lighting
    _settings.led_count      = prefs.getInt("l_cnt", _settings.led_count);
    _settings.led_brightness = prefs.getInt("l_brt", _settings.led_brightness);

    // WiFi
    _settings.wifi_mode      = prefs.getInt("w_mode", _settings.wifi_mode);
    String ssid = prefs.getString("w_ssid", _settings.wifi_ssid);
    String pass = prefs.getString("w_pass", _settings.wifi_password);
    strncpy(_settings.wifi_ssid, ssid.c_str(), sizeof(_settings.wifi_ssid) - 1);
    strncpy(_settings.wifi_password, pass.c_str(), sizeof(_settings.wifi_password) - 1);

    prefs.end();
    Serial.println("[CONFIG] Loaded settings from NVS");
}

void ConfigManager::_saveToNVS() {
    Preferences prefs;
    if (!prefs.begin("claw_cfg", false)) { // read-write
        Serial.println("[CONFIG] ERROR: Failed to open NVS for writing");
        return;
    }

    // Motion
    prefs.putFloat("spm", _settings.steps_per_mm);
    prefs.putFloat("x_max", _settings.x_max_mm);
    prefs.putFloat("x_spd", _settings.x_max_speed);
    prefs.putFloat("x_acc", _settings.x_acceleration);
    prefs.putFloat("y_max", _settings.y_max_mm);
    prefs.putFloat("y_spd", _settings.y_max_speed);
    prefs.putFloat("y_acc", _settings.y_acceleration);
    prefs.putFloat("z_max", _settings.z_max_mm);
    prefs.putFloat("z_spd", _settings.z_max_speed);
    prefs.putFloat("z_acc", _settings.z_acceleration);
    prefs.putFloat("z_spm", _settings.z_steps_per_mm);
    prefs.putBool("inv_a", _settings.invert_motor_a);
    prefs.putBool("inv_b", _settings.invert_motor_b);
    prefs.putBool("inv_z", _settings.invert_motor_z);

    // Homing
    prefs.putFloat("xh_spd", _settings.x_home_speed);
    prefs.putFloat("yh_spd", _settings.y_home_speed);
    prefs.putFloat("zh_spd", _settings.z_home_speed);
    prefs.putFloat("xh_bo", _settings.x_home_backoff);
    prefs.putFloat("yh_bo", _settings.y_home_backoff);
    prefs.putFloat("zh_bo", _settings.z_home_backoff);

    // Drop / Park
    prefs.putFloat("drp_x", _settings.drop_x);
    prefs.putFloat("drp_y", _settings.drop_y);
    prefs.putFloat("prk_x", _settings.park_x);
    prefs.putFloat("prk_y", _settings.park_y);

    // Claw
    prefs.putInt("c_open", _settings.claw_open_angle);
    prefs.putInt("c_close", _settings.claw_close_angle);
    prefs.putInt("c_part", _settings.claw_partial_angle);
    prefs.putInt("c_grip", _settings.claw_grip_delay_ms);

    // Joystick
    prefs.putInt("jx_c", _settings.joy_x_center);
    prefs.putInt("jy_c", _settings.joy_y_center);
    prefs.putInt("jz_c", _settings.joy_z_center);
    prefs.putInt("j_dz", _settings.joy_deadzone);
    prefs.putFloat("j_sens", _settings.joy_sensitivity);
    prefs.putFloat("j_expo", _settings.joy_expo);
    prefs.putBool("jinv_x", _settings.joy_invert_x);
    prefs.putBool("jinv_y", _settings.joy_invert_y);
    prefs.putBool("jinv_z", _settings.joy_invert_z);

    // Game
    prefs.putInt("g_dur", _settings.game_duration_s);
    prefs.putInt("g_cred", _settings.credits_per_game);
    prefs.putBool("g_free", _settings.free_play);
    prefs.putInt("g_str", _settings.claw_strength);
    prefs.putBool("g_ahome", _settings.auto_home_on_boot);
    prefs.putBool("g_rehome", _settings.re_home_between_games);

    // Lighting
    prefs.putInt("l_cnt", _settings.led_count);
    prefs.putInt("l_brt", _settings.led_brightness);

    // WiFi
    prefs.putInt("w_mode", _settings.wifi_mode);
    prefs.putString("w_ssid", _settings.wifi_ssid);
    prefs.putString("w_pass", _settings.wifi_password);

    prefs.end();
    Serial.println("[CONFIG] Settings saved to NVS");
}

bool ConfigManager::saveAll() {
    _saveToNVS();
    return true;
}

bool ConfigManager::saveMotion() { _saveToNVS(); return true; }
bool ConfigManager::saveJoystick() { _saveToNVS(); return true; }
bool ConfigManager::saveClaw() { _saveToNVS(); return true; }
bool ConfigManager::saveGame() { _saveToNVS(); return true; }
bool ConfigManager::saveLighting() { _saveToNVS(); return true; }
bool ConfigManager::saveWifi() { _saveToNVS(); return true; }

void ConfigManager::resetToDefaults() {
    loadDefaults();
    _saveToNVS();
    Serial.println("[CONFIG] Reset to factory defaults");
}

bool ConfigManager::applyFromJson(const char* json) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("[CONFIG] JSON parse error: %s\n", err.c_str());
        return false;
    }

    // Motion
    if (doc.containsKey("steps_per_mm"))    _settings.steps_per_mm = doc["steps_per_mm"];
    if (doc.containsKey("x_max_mm"))        _settings.x_max_mm = doc["x_max_mm"];
    if (doc.containsKey("x_max_speed"))     _settings.x_max_speed = doc["x_max_speed"];
    if (doc.containsKey("x_acceleration"))  _settings.x_acceleration = doc["x_acceleration"];
    if (doc.containsKey("y_max_mm"))        _settings.y_max_mm = doc["y_max_mm"];
    if (doc.containsKey("y_max_speed"))     _settings.y_max_speed = doc["y_max_speed"];
    if (doc.containsKey("y_acceleration"))  _settings.y_acceleration = doc["y_acceleration"];
    if (doc.containsKey("z_max_mm"))        _settings.z_max_mm = doc["z_max_mm"];
    if (doc.containsKey("z_max_speed"))     _settings.z_max_speed = doc["z_max_speed"];
    if (doc.containsKey("z_acceleration"))  _settings.z_acceleration = doc["z_acceleration"];
    if (doc.containsKey("z_steps_per_mm"))  _settings.z_steps_per_mm = doc["z_steps_per_mm"];
    if (doc.containsKey("invert_motor_a"))  _settings.invert_motor_a = doc["invert_motor_a"];
    if (doc.containsKey("invert_motor_b"))  _settings.invert_motor_b = doc["invert_motor_b"];
    if (doc.containsKey("invert_motor_z"))  _settings.invert_motor_z = doc["invert_motor_z"];

    // Claw
    if (doc.containsKey("claw_open_angle"))     _settings.claw_open_angle = doc["claw_open_angle"];
    if (doc.containsKey("claw_close_angle"))    _settings.claw_close_angle = doc["claw_close_angle"];
    if (doc.containsKey("claw_partial_angle"))  _settings.claw_partial_angle = doc["claw_partial_angle"];
    if (doc.containsKey("claw_grip_delay_ms"))  _settings.claw_grip_delay_ms = doc["claw_grip_delay_ms"];

    // Joystick
    if (doc.containsKey("joy_x_center"))    _settings.joy_x_center = doc["joy_x_center"];
    if (doc.containsKey("joy_y_center"))    _settings.joy_y_center = doc["joy_y_center"];
    if (doc.containsKey("joy_z_center"))    _settings.joy_z_center = doc["joy_z_center"];
    if (doc.containsKey("joy_deadzone"))    _settings.joy_deadzone = doc["joy_deadzone"];
    if (doc.containsKey("joy_sensitivity")) _settings.joy_sensitivity = doc["joy_sensitivity"];
    if (doc.containsKey("joy_expo"))        _settings.joy_expo = doc["joy_expo"];
    if (doc.containsKey("joy_invert_x"))    _settings.joy_invert_x = doc["joy_invert_x"];
    if (doc.containsKey("joy_invert_y"))    _settings.joy_invert_y = doc["joy_invert_y"];
    if (doc.containsKey("joy_invert_z"))    _settings.joy_invert_z = doc["joy_invert_z"];

    // Game
    if (doc.containsKey("game_duration_s"))     _settings.game_duration_s = doc["game_duration_s"];
    if (doc.containsKey("credits_per_game"))    _settings.credits_per_game = doc["credits_per_game"];
    if (doc.containsKey("free_play"))           _settings.free_play = doc["free_play"];
    if (doc.containsKey("claw_strength"))       _settings.claw_strength = doc["claw_strength"];

    // Lighting
    if (doc.containsKey("led_count"))       _settings.led_count = doc["led_count"];
    if (doc.containsKey("led_brightness"))  _settings.led_brightness = doc["led_brightness"];

    _saveToNVS();
    return true;
}

String ConfigManager::toJson() const {
    JsonDocument doc;

    doc["steps_per_mm"]     = _settings.steps_per_mm;
    doc["x_max_mm"]         = _settings.x_max_mm;
    doc["x_max_speed"]      = _settings.x_max_speed;
    doc["x_acceleration"]   = _settings.x_acceleration;
    doc["y_max_mm"]         = _settings.y_max_mm;
    doc["y_max_speed"]      = _settings.y_max_speed;
    doc["y_acceleration"]   = _settings.y_acceleration;
    doc["z_max_mm"]         = _settings.z_max_mm;
    doc["z_max_speed"]      = _settings.z_max_speed;
    doc["z_acceleration"]   = _settings.z_acceleration;
    doc["z_steps_per_mm"]   = _settings.z_steps_per_mm;
    doc["invert_motor_a"]   = _settings.invert_motor_a;
    doc["invert_motor_b"]   = _settings.invert_motor_b;
    doc["invert_motor_z"]   = _settings.invert_motor_z;

    doc["claw_open_angle"]      = _settings.claw_open_angle;
    doc["claw_close_angle"]     = _settings.claw_close_angle;
    doc["claw_partial_angle"]   = _settings.claw_partial_angle;
    doc["claw_grip_delay_ms"]   = _settings.claw_grip_delay_ms;

    doc["joy_x_center"]     = _settings.joy_x_center;
    doc["joy_y_center"]     = _settings.joy_y_center;
    doc["joy_z_center"]     = _settings.joy_z_center;
    doc["joy_deadzone"]     = _settings.joy_deadzone;
    doc["joy_sensitivity"]  = _settings.joy_sensitivity;
    doc["joy_expo"]         = _settings.joy_expo;
    doc["joy_invert_x"]     = _settings.joy_invert_x;
    doc["joy_invert_y"]     = _settings.joy_invert_y;
    doc["joy_invert_z"]     = _settings.joy_invert_z;

    doc["game_duration_s"]      = _settings.game_duration_s;
    doc["credits_per_game"]     = _settings.credits_per_game;
    doc["free_play"]            = _settings.free_play;
    doc["claw_strength"]        = _settings.claw_strength;

    doc["led_count"]        = _settings.led_count;
    doc["led_brightness"]   = _settings.led_brightness;

    String output;
    serializeJson(doc, output);
    return output;
}

#else
// Native build stubs for unit testing
bool ConfigManager::begin() { _initialized = true; return true; }
void ConfigManager::_loadFromNVS() {}
void ConfigManager::_saveToNVS() {}
bool ConfigManager::saveAll() { return true; }
bool ConfigManager::saveMotion() { return true; }
bool ConfigManager::saveJoystick() { return true; }
bool ConfigManager::saveClaw() { return true; }
bool ConfigManager::saveGame() { return true; }
bool ConfigManager::saveLighting() { return true; }
bool ConfigManager::saveWifi() { return true; }
void ConfigManager::resetToDefaults() { loadDefaults(); }
bool ConfigManager::applyFromJson(const char*) { return true; }
String ConfigManager::toJson() const { return "{}"; }
#endif
