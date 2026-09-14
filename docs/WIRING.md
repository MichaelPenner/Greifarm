# Claw Machine Wiring Guide

## Core Components
- **Microcontroller**: ESP32 (Wemos D1 R32 recommended for direct CNC Shield compatibility)
- **Shield**: CNC Shield V3
- **Stepper Drivers**: 3x A4988
- **Power**: 12V PSU (Motors), 5V PSU (ESP32, Sensors, LED, Servo)

## WARNINGS & PRECAUTIONS
- ⚠️ **CNC Shield V3 Mod**: You MUST remove the `R1` resistor from the CNC shield before mounting the ESP32. If not removed, the ESP32 will fail to boot due to strapping pin conflicts.
- ⚠️ **Power Separation**: DO NOT power the Servo or WS2812 LEDs from the ESP32's 5V pin. They require more current than the ESP32 regulator can provide. Use a dedicated 5V power supply.
- ⚠️ **Grounding**: Ensure all grounds (12V PSU, 5V PSU, ESP32, CNC Shield) are connected together.

---

## 1. ESP32 to CNC Shield Mapping

If using a standard ESP32 DevKit (not D1 R32), you must manually wire the ESP32 to the CNC Shield pins:

| CNC Shield Pin | ESP32 GPIO | Function |
| :--- | :--- | :--- |
| D2 | GPIO 26 | X-Step (CoreXY Motor A) |
| D5 | GPIO 14 | X-Dir (CoreXY Motor A) |
| D3 | GPIO 25 | Y-Step (CoreXY Motor B) |
| D6 | GPIO 27 | Y-Dir (CoreXY Motor B) |
| D4 | GPIO 12 | Z-Step (Lift Motor) |
| D7 | GPIO 33 | Z-Dir (Lift Motor) |
| D8 | GPIO 13 | Stepper Enable (Active LOW) |

## 2. Limit Switches & Safety

Wire these directly to the ESP32. We use Normally Closed (NC) switches wired to ground for fail-safe operation (if a wire breaks, it triggers).

| Sensor | ESP32 GPIO | CNC Shield (if D1 R32) | Wiring |
| :--- | :--- | :--- | :--- |
| X-Home (Left) | GPIO 2 | D9 (X+) | Switch NC -> GPIO 2, Switch COM -> GND |
| Y-Home (Front) | GPIO 4 | D10 (Y+) | Switch NC -> GPIO 4, Switch COM -> GND |
| Z-Top (Up) | GPIO 5 | D11 (Z+) | Switch NC -> GPIO 5, Switch COM -> GND |
| Z-Bottom (Down)| GPIO 15 | D12 (SpnEn) | Switch NC -> GPIO 15, Switch COM -> GND |
| Emergency Stop | GPIO 19 | - | Button NC -> GPIO 19, Button COM -> GND |

## 3. Joystick (JH-D400X-R4)

The joystick has 3 potentiometers (X, Y, Z-twist) and a button.
**Important**: Wire the potentiometers to 3.3V, NOT 5V, to protect the ESP32 ADCs!

| Joystick Pin | ESP32 GPIO | Description |
| :--- | :--- | :--- |
| X-Axis Wiper | GPIO 36 (VP) | Left/Right |
| Y-Axis Wiper | GPIO 39 (VN) | Forward/Back |
| Z-Axis Wiper | GPIO 34 | Claw twist (if used) |
| Button | GPIO 35 | Claw Drop |
| VCC | 3.3V | Power for Pots |
| GND | GND | Ground for Pots |
| Button GND | GND | Ground for button |

## 4. Claw Servo & LEDs

| Component | ESP32 GPIO | Power |
| :--- | :--- | :--- |
| Claw Servo PWM | GPIO 16 | 5V PSU directly |
| WS2812B Data | GPIO 17 | 5V PSU directly (Use 330Ω inline resistor) |

## 5. Prize Detection (Break-beam)

| Sensor Pin | ESP32 GPIO | Power |
| :--- | :--- | :--- |
| Receiver OUT | GPIO 18 | 5V/3.3V depending on sensor spec |
| Receiver GND | GND | GND |
| Transmitter VCC| - | 5V |
| Transmitter GND| - | GND |
