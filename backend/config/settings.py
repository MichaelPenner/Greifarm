"""
Claw Machine Backend — Settings (Pydantic)
"""
from pydantic_settings import BaseSettings
from pydantic import Field
from pathlib import Path


class Settings(BaseSettings):
    """Application settings loaded from environment or .env file."""

    # Server
    host: str = "0.0.0.0"
    port: int = 8000
    debug: bool = False

    # ESP32 connection
    esp_host: str = "192.168.178.136"
    esp_ws_port: int = 81
    esp_ws_path: str = "/ws"
    esp_reconnect_interval: float = 3.0

    # Physical Machine Tuning (Passed from HA Addon Config)
    steps_per_mm_xy: float = 640.0
    steps_per_mm_z: float = 50.0
    max_speed_xy: float = 150.0
    max_speed_z: float = 50.0
    max_mm_x: float = 220.0
    max_mm_y: float = 220.0
    max_mm_z: float = 200.0

    # Database
    db_path: str = str(Path(__file__).parent.parent / "data" / "clawmachine.db")

    # Security
    admin_pin: str = "1234"
    secret_key: str = "change-me-in-production"

    # Simulation
    simulation_mode: bool = False

    # Sound
    sounds_dir: str = str(Path(__file__).parent.parent.parent / "frontend" / "sounds")

    # Frontend
    frontend_dir: str = str(Path(__file__).parent.parent.parent / "frontend")

    model_config = {
        "env_file": ".env",
        "env_prefix": "CLAW_",
    }


settings = Settings()
