# Frontend Setup & Documentation

The frontend is built using standard web technologies (HTML, CSS, JS) without any build steps (no npm, webpack, or react). This keeps it lightweight and perfect for serving from a Raspberry Pi.

## Architecture

- **`index.html`**: The main Player UI. Designed for a tablet screen mounted on the machine.
- **`dashboard.html`**: The Admin interface. Accessible via browser on the same network to control and configure the machine.
- **`css/design-system.css`**: Contains all CSS custom properties (colors, spacing, typography) and utility classes.
- **`js/websocket-client.js`**: Handles the persistent connection to the FastAPI backend.
- **`js/state-manager.js`**: A reactive state store that updates when telemetry arrives and notifies UI components.
- **`js/utils/audio-manager.js`**: Handles sound playback for game events.

## Installation

The frontend files are served directly by the FastAPI backend. 
Ensure the `frontend_dir` setting in `backend/config/settings.py` points to this directory.

```bash
# To run the backend and serve the frontend:
cd backend
pip install -r requirements.txt
python main.py
```

Then open a browser to:
- Player UI: `http://localhost:8000/`
- Dashboard: `http://localhost:8000/dashboard.html`

## Adding Custom Sounds

Place your MP3 files in the `frontend/sounds/` directory. The expected files are:
- `game_start.mp3`
- `countdown.mp3`
- `claw_close.mp3`
- `win.mp3`
- `loss.mp3`
- `error.mp3`
- `click.mp3` (for UI buttons)

If a sound is missing, the system will gracefully log a warning in the console but continue functioning.
