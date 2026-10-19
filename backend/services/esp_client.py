"""
ESP WebSocket Client — Connects backend to ESP32 real-time controller.
"""
import asyncio
import json
import logging
from typing import Optional, Callable, Any
import websockets
from config.settings import settings

logger = logging.getLogger("esp_client")


class ESPClient:
    """
    Maintains a persistent WebSocket connection to the ESP32.
    Receives telemetry and forwards commands.
    """

    def __init__(self):
        self._ws: Optional[websockets.WebSocketClientProtocol] = None
        self._connected = False
        self._telemetry: dict = {}
        self._listeners: list[Callable] = []
        self._running = False
        self._reconnect_task: Optional[asyncio.Task] = None

    @property
    def connected(self) -> bool:
        return self._connected

    @property
    def telemetry(self) -> dict:
        return self._telemetry.copy()

    def on_message(self, callback: Callable):
        """Register a callback for incoming ESP messages."""
        self._listeners.append(callback)

    async def start(self):
        """Start the persistent connection loop."""
        self._running = True
        self._reconnect_task = asyncio.create_task(self._connection_loop())
        logger.info("ESP client started")

    async def stop(self):
        """Stop the connection."""
        self._running = False
        if self._ws:
            await self._ws.close()
        if self._reconnect_task:
            self._reconnect_task.cancel()

    async def send_command(self, command: dict) -> bool:
        """Send a command to the ESP32."""
        if not self._connected or not self._ws:
            logger.warning("Cannot send command — not connected to ESP")
            return False

        try:
            await self._ws.send(json.dumps(command))
            return True
        except Exception as e:
            logger.error(f"Failed to send command: {e}")
            self._connected = False
            return False

    async def _connection_loop(self):
        """Reconnecting WebSocket client loop."""
        uri = f"ws://{settings.esp_host}:{settings.esp_ws_port}{settings.esp_ws_path}"

        while self._running:
            try:
                logger.info(f"Connecting to ESP at {uri}...")
                async with websockets.connect(uri, ping_interval=2, ping_timeout=5) as ws:
                    self._ws = ws
                    self._connected = True
                    logger.info("Connected to ESP32!")

                    # Push config from Home Assistant to ESP32
                    try:
                        await ws.send(json.dumps({
                            "type": "set_config",
                            "config": {
                                "steps_per_mm": settings.steps_per_mm_xy,
                                "z_steps_per_mm": settings.steps_per_mm_z,
                                "x_max_speed": settings.max_speed_xy,
                                "y_max_speed": settings.max_speed_xy,
                                "z_max_speed": settings.max_speed_z,
                                "x_max_mm": settings.max_mm_x,
                                "y_max_mm": settings.max_mm_y,
                                "z_max_mm": settings.max_mm_z
                            }
                        }))
                        logger.info("Pushed HA Addon configuration to ESP32.")
                    except Exception as e:
                        logger.error(f"Failed to push config: {e}")

                    # Send initial heartbeat
                    await ws.send(json.dumps({"type": "heartbeat"}))

                    async for message in ws:
                        try:
                            data = json.loads(message)
                            msg_type = data.get("type", "")

                            # Update local telemetry cache
                            if msg_type == "telemetry":
                                self._telemetry = data

                            # Notify listeners
                            for listener in self._listeners:
                                try:
                                    if asyncio.iscoroutinefunction(listener):
                                        await listener(data)
                                    else:
                                        listener(data)
                                except Exception as e:
                                    logger.error(f"Listener error: {e}")

                        except json.JSONDecodeError:
                            logger.warning(f"Invalid JSON from ESP: {message[:100]}")

            except (websockets.exceptions.ConnectionClosed,
                    ConnectionRefusedError, OSError) as e:
                logger.warning(f"ESP connection lost: {e}")
                self._connected = False

            except Exception as e:
                logger.error(f"ESP client error: {e}")
                self._connected = False

            if self._running:
                logger.info(f"Reconnecting in {settings.esp_reconnect_interval}s...")
                await asyncio.sleep(settings.esp_reconnect_interval)

        self._connected = False


# Simulation client for development without hardware
class SimulatedESPClient(ESPClient):
    """Fake ESP client that simulates telemetry for UI development."""

    def __init__(self):
        super().__init__()
        self._sim_state = "IDLE"
        self._sim_x = 100.0
        self._sim_y = 100.0
        self._sim_z = 0.0
        self._sim_credits = 999

    async def start(self):
        self._running = True
        self._connected = True
        self._reconnect_task = asyncio.create_task(self._simulate_loop())
        logger.info("Simulated ESP client started")

    async def send_command(self, command: dict) -> bool:
        cmd_type = command.get("type", "")
        logger.info(f"[SIM] Command: {cmd_type}")

        if cmd_type == "game_start":
            self._sim_state = "PLAYING"
        elif cmd_type == "game_stop":
            self._sim_state = "IDLE"
        elif cmd_type == "home":
            self._sim_state = "HOMING"
            await asyncio.sleep(2)
            self._sim_state = "IDLE"
            self._sim_x = 0
            self._sim_y = 0
            self._sim_z = 0
        elif cmd_type == "claw_open":
            pass
        elif cmd_type == "claw_close":
            pass

        return True

    async def _simulate_loop(self):
        import random
        while self._running:
            self._telemetry = {
                "type": "telemetry",
                "state": self._sim_state,
                "x": self._sim_x + random.uniform(-0.5, 0.5),
                "y": self._sim_y + random.uniform(-0.5, 0.5),
                "z": self._sim_z,
                "homed": True,
                "moving": False,
                "claw": 1,
                "claw_angle": 30,
                "joy_x": 0, "joy_y": 0, "joy_z": 0, "joy_btn": False,
                "sensors": 0,
                "estop": False,
                "prize_beam": False,
                "credits": self._sim_credits,
                "game_time": 0,
                "games_played": 42,
                "games_won": 7,
                "safety": 0,
                "motors_on": True,
                "uptime": 123456,
            }

            for listener in self._listeners:
                try:
                    if asyncio.iscoroutinefunction(listener):
                        await listener(self._telemetry)
                    else:
                        listener(self._telemetry)
                except Exception:
                    pass

            await asyncio.sleep(0.1)


def create_esp_client() -> ESPClient:
    """Factory: returns simulated or real ESP client based on settings."""
    if settings.simulation_mode:
        return SimulatedESPClient()
    return ESPClient()
