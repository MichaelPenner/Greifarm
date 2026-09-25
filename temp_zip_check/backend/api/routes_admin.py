"""
API Routes — Admin (PIN-protected)
"""
from fastapi import APIRouter, HTTPException, Header
from config.settings import settings

router = APIRouter()


def verify_admin(x_admin_pin: str = Header(default="")):
    """Simple PIN-based admin authentication."""
    if x_admin_pin != settings.admin_pin:
        raise HTTPException(status_code=403, detail="Invalid admin PIN")


@router.post("/admin/config", dependencies=[])
async def set_config(config: dict, x_admin_pin: str = Header(default="")):
    """Update ESP machine configuration."""
    verify_admin(x_admin_pin)
    from main import esp_client
    success = await esp_client.send_command({"type": "set_config", "config": config})
    return {"success": success}


@router.get("/admin/config")
async def get_config(x_admin_pin: str = Header(default="")):
    """Get current ESP machine configuration."""
    verify_admin(x_admin_pin)
    from main import esp_client
    success = await esp_client.send_command({"type": "get_config"})
    return {"success": success, "note": "Config will arrive via WebSocket"}


@router.post("/admin/save-config")
async def save_config(x_admin_pin: str = Header(default="")):
    """Save current config to ESP NVS."""
    verify_admin(x_admin_pin)
    from main import esp_client
    success = await esp_client.send_command({"type": "save_config"})
    return {"success": success}


@router.post("/admin/reset-config")
async def reset_config(x_admin_pin: str = Header(default="")):
    """Reset config to factory defaults."""
    verify_admin(x_admin_pin)
    from main import esp_client
    success = await esp_client.send_command({"type": "reset_config"})
    return {"success": success}


@router.post("/admin/calibrate/joystick-center")
async def calibrate_joystick_center(x_admin_pin: str = Header(default="")):
    """Calibrate joystick center position."""
    verify_admin(x_admin_pin)
    from main import esp_client
    success = await esp_client.send_command({"type": "joy_cal_center"})
    return {"success": success}


@router.post("/admin/calibrate/joystick-range-start")
async def cal_joy_range_start(x_admin_pin: str = Header(default="")):
    verify_admin(x_admin_pin)
    from main import esp_client
    success = await esp_client.send_command({"type": "joy_cal_range_start"})
    return {"success": success}


@router.post("/admin/calibrate/joystick-range-stop")
async def cal_joy_range_stop(x_admin_pin: str = Header(default="")):
    verify_admin(x_admin_pin)
    from main import esp_client
    success = await esp_client.send_command({"type": "joy_cal_range_stop"})
    return {"success": success}


@router.get("/admin/logs")
async def get_logs(x_admin_pin: str = Header(default="")):
    """Request ESP log buffer."""
    verify_admin(x_admin_pin)
    from main import esp_client
    success = await esp_client.send_command({"type": "get_logs"})
    return {"success": success, "note": "Logs will arrive via WebSocket"}


@router.post("/admin/set-position")
async def set_position(x: float = 0, y: float = 0, z: float = 0,
                       x_admin_pin: str = Header(default="")):
    """Set current position (for calibration)."""
    verify_admin(x_admin_pin)
    from main import esp_client
    success = await esp_client.send_command({
        "type": "set_position", "x": x, "y": y, "z": z
    })
    return {"success": success}
