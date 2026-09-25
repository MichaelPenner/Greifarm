#pragma once
// =============================================================
// Pin Configuration — Claw Machine ESP32
// =============================================================
// Board: Wemos D1 R32 (ESP32 in Uno form factor) + CNC Shield V3
//
// IMPORTANT: Remove R1 resistor from CNC Shield before use!
//
// Verification status:
//   ✓  = Confirmed by datasheet / research
//   ?  = Assumed — verify with multimeter before powering motors
//   ?? = Needs verification — unknown, placeholder
// =============================================================

#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// =============================================================
// Stepper Motor Pins (via CNC Shield V3)
// These map through the D1 R32 Uno-header to ESP32 GPIOs
// =============================================================

// Motor A (CoreXY) — mapped to CNC Shield X-axis slot
// Status: ? — verify D1 R32 pin mapping with multimeter
#define PIN_MOTOR_A_STEP    26  // CNC Shield D2 → GPIO 26
#define PIN_MOTOR_A_DIR     14  // CNC Shield D5 → GPIO 14

// Motor B (CoreXY) — mapped to CNC Shield Y-axis slot
// Status: ? — verify D1 R32 pin mapping with multimeter
#define PIN_MOTOR_B_STEP    25  // CNC Shield D3 → GPIO 25
#define PIN_MOTOR_B_DIR     27  // CNC Shield D6 → GPIO 27

// Motor Z (Claw lift) — mapped to CNC Shield Z-axis slot
// Status: ? — verify D1 R32 pin mapping with multimeter
#define PIN_MOTOR_Z_STEP    12  // CNC Shield D4 → GPIO 12
#define PIN_MOTOR_Z_DIR     33  // CNC Shield D7 → GPIO 33

// Stepper Enable — shared for all drivers on CNC Shield
// Active LOW — pull LOW to enable motors
// Status: ? — verify; GPIO 13 may vary by D1 R32 revision
#define PIN_STEPPER_ENABLE  13  // CNC Shield D8 → GPIO 13

// =============================================================
// Limit Switches (directly wired or via CNC Shield endstop headers)
// All switches: NC (normally closed) wiring recommended
// Pin goes LOW when switch is triggered (wire breaks = triggered = safe)
// =============================================================

// Status: ✓ — existing switch on Sapphire S left side
#define PIN_LIMIT_X_HOME    2   // CNC Shield D9 → GPIO 2

// Status: ?? — new switch, needs to be installed + wired
#define PIN_LIMIT_Y_HOME    4   // CNC Shield D10 → GPIO 4

// Status: ?? — new switch, needs to be installed + wired
#define PIN_LIMIT_Z_TOP     5   // CNC Shield D11 → GPIO 5

// Status: ?? — new switch, needs to be installed + wired
#define PIN_LIMIT_Z_BOTTOM  15  // CNC Shield D12 → GPIO 15

// =============================================================
// Claw Servo
// IMPORTANT: Do NOT power servo from ESP32 — use external 5V supply
// =============================================================

// Status: ✓ — standard PWM-capable GPIO
#define PIN_CLAW_SERVO      16  // Direct wire to ESP32 GPIO 16

// =============================================================
// WS2812B LED Strip
// Use 330Ω–470Ω series resistor on data line
// Power LEDs from external 5V supply, NOT from ESP32
// =============================================================

// Status: ✓ — standard digital output GPIO
#define PIN_WS2812_DATA     17  // Direct wire to ESP32 GPIO 17

// =============================================================
// Joystick (JH-D400X-R4)
// 4-axis potentiometer joystick with button
// Potentiometers: 10kΩ
//
// VOLTAGE NOTE:
//   If powering pots from 5V → MUST add voltage dividers (2× 10kΩ)
//   If powering pots from 3.3V → connect directly (reduced range but safe)
//   RECOMMENDED: Power from 3.3V for simplicity
//
// All analog pins must be on ADC1 (ADC2 conflicts with WiFi)
// =============================================================

// Status: ✓ — ADC1 input-only pins
#define PIN_JOYSTICK_X      36  // GPIO 36 (VP) — ADC1_CH0
#define PIN_JOYSTICK_Y      39  // GPIO 39 (VN) — ADC1_CH3
#define PIN_JOYSTICK_Z      34  // GPIO 34       — ADC1_CH6

// Status: ✓ — digital input with internal pull-up
#define PIN_JOYSTICK_BTN    35  // GPIO 35       — input only

// =============================================================
// Prize Detection Sensor (break-beam / optical)
// IR transmitter + receiver pair in prize chute
// Active LOW when beam is broken (prize passing through)
// =============================================================

// Status: ✓ — standard digital GPIO
#define PIN_PRIZE_SENSOR    18  // Direct wire to ESP32 GPIO 18

// =============================================================
// Emergency Stop Button
// MUST use NC (normally closed) wiring
// Pin is HIGH when E-stop is NOT pressed (circuit closed)
// Pin goes LOW when E-stop IS pressed (circuit opens)
// A broken wire also reads as LOW → fail-safe
// =============================================================

// Status: ✓ — standard digital GPIO
#define PIN_ESTOP           19  // Direct wire to ESP32 GPIO 19

// =============================================================
// Optional / Future Expansion
// =============================================================

// Buzzer for local audio feedback (optional)
#define PIN_BUZZER          23  // GPIO 23 — PWM capable

// Spare GPIO on CNC Shield SpnEn header repurposed for Coin Acceptor
#define PIN_COIN_ACCEPTOR   32  // GPIO 32 — open-collector pulse from PY-800VI

// =============================================================
// Pin validation — compile-time checks
// =============================================================

// Ensure analog pins are on ADC1 (ADC2 not usable with WiFi)
static_assert(PIN_JOYSTICK_X == 36 || PIN_JOYSTICK_X == 39 ||
              PIN_JOYSTICK_X == 34 || PIN_JOYSTICK_X == 35 ||
              PIN_JOYSTICK_X == 32 || PIN_JOYSTICK_X == 33,
              "Joystick X must be on ADC1 channel (GPIO 32-36, 39)");

#endif // PIN_CONFIG_H
