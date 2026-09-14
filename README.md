# Project root readme
# DIY CoreXY Claw Machine

This project converts a Two Trees Sapphire S CoreXY 3D printer into an arcade claw machine.

## Directory Structure
- `firmware/` - ESP32 PlatformIO project. Flashes the microcontroller.
- `backend/` - Raspberry Pi FastAPI backend. Connects to ESP32 over WebSocket.
- `frontend/` - Tablet UI (HTML/JS/CSS) served by the backend.
- `docs/` - Extensive documentation for wiring, setup, and architecture.
- `deploy/` - Scripts for Raspberry Pi installation.

## Quick Start
1. Read `docs/WIRING.md` and assemble the hardware.
2. Open `firmware/` in PlatformIO and flash the ESP32.
3. On your Raspberry Pi, run `deploy/setup_pi.sh`.

See `docs/ARCHITECTURE.md` for a full system overview.
