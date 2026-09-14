"""
Claw Machine Backend — FastAPI Main Application
"""
import asyncio
import logging
from contextlib import asynccontextmanager
from pathlib import Path

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.staticfiles import StaticFiles
from fastapi.middleware.cors import CORSMiddleware

from config.settings import settings
from database.models import init_db
from services.esp_client import create_esp_client
from api.routes_game import router as game_router
from api.routes_machine import router as machine_router
from api.routes_stats import router as stats_router
from api.routes_admin import router as admin_router

# Configure logging
logging.basicConfig(
    level=logging.DEBUG if settings.debug else logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s"
)
logger = logging.getLogger("main")

# Global ESP client instance
esp_client = create_esp_client()

# Connected browser WebSocket clients
browser_clients: set[WebSocket] = set()


async def broadcast_to_browsers(data: dict):
    """Forward ESP telemetry to all connected browser clients."""
    import json
    message = json.dumps(data)
    disconnected = set()
    for ws in browser_clients:
        try:
            await ws.send_text(message)
        except Exception:
            disconnected.add(ws)
    browser_clients -= disconnected


@asynccontextmanager
async def lifespan(app: FastAPI):
    """Application startup and shutdown."""
    logger.info("Starting Claw Machine Backend...")

    # Initialize database
    await init_db()
    logger.info("Database initialized")

    # Start ESP client connection
    esp_client.on_message(broadcast_to_browsers)
    await esp_client.start()
    logger.info(f"ESP client started (simulation={settings.simulation_mode})")

    yield

    # Shutdown
    await esp_client.stop()
    logger.info("Backend stopped")


# Create FastAPI app
app = FastAPI(
    title="Claw Machine API",
    version="1.0.0",
    description="Backend API for DIY CoreXY Claw Machine",
    lifespan=lifespan,
)

# CORS — allow all origins for local network use
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Mount API routers
app.include_router(game_router, prefix="/api", tags=["Game"])
app.include_router(machine_router, prefix="/api", tags=["Machine"])
app.include_router(stats_router, prefix="/api", tags=["Statistics"])
app.include_router(admin_router, prefix="/api", tags=["Admin"])


# WebSocket proxy — Browser ↔ Backend ↔ ESP32
@app.websocket("/ws")
async def websocket_proxy(websocket: WebSocket):
    """
    Browser WebSocket endpoint.
    - Receives commands from browser → forwards to ESP32
    - Receives telemetry from ESP32 → forwards to browser (via listener)
    """
    await websocket.accept()
    browser_clients.add(websocket)
    logger.info(f"Browser client connected ({len(browser_clients)} total)")

    try:
        while True:
            # Receive commands from browser
            data = await websocket.receive_text()
            import json
            try:
                command = json.loads(data)
                cmd_type = command.get("type", "unknown")

                # Heartbeat handling
                if cmd_type == "heartbeat":
                    await websocket.send_text(json.dumps({
                        "type": "heartbeat_ack",
                        "esp_connected": esp_client.connected,
                    }))
                    continue

                # Forward command to ESP
                success = await esp_client.send_command(command)
                if not success:
                    await websocket.send_text(json.dumps({
                        "type": "nack",
                        "cmd": cmd_type,
                        "reason": "ESP not connected"
                    }))

            except json.JSONDecodeError:
                await websocket.send_text(json.dumps({
                    "type": "nack",
                    "reason": "Invalid JSON"
                }))

    except WebSocketDisconnect:
        pass
    except Exception as e:
        logger.error(f"WebSocket error: {e}")
    finally:
        browser_clients.discard(websocket)
        logger.info(f"Browser client disconnected ({len(browser_clients)} remaining)")


# Machine status endpoint (quick REST check)
@app.get("/api/status")
async def get_status():
    """Get current machine status."""
    telemetry = esp_client.telemetry
    return {
        "online": True,
        "esp_connected": esp_client.connected,
        "state": telemetry.get("state", "UNKNOWN"),
        "simulation": settings.simulation_mode,
    }


# Serve frontend static files
frontend_path = Path(settings.frontend_dir)
if frontend_path.exists():
    app.mount("/", StaticFiles(directory=str(frontend_path), html=True), name="frontend")
else:
    logger.warning(f"Frontend directory not found: {frontend_path}")


if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host=settings.host, port=settings.port,
                reload=settings.debug, log_level="info")
