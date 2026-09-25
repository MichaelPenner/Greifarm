"""
API Routes — Machine Control
"""
from fastapi import APIRouter

router = APIRouter()


@router.post("/machine/home")
async def home_machine(axis: str = "all"):
    """Home the machine (all axes or specific axis)."""
    from main import esp_client
    success = await esp_client.send_command({"type": "home", "axis": axis})
    return {"success": success}


@router.post("/machine/stop")
async def stop_machine():
    """Controlled stop — decelerates all axes."""
    from main import esp_client
    success = await esp_client.send_command({"type": "stop"})
    return {"success": success}


@router.post("/machine/estop")
async def emergency_stop():
    """Emergency stop — immediately halts all motors."""
    from main import esp_client
    success = await esp_client.send_command({"type": "estop"})
    return {"success": success}


@router.post("/machine/reset")
async def reset_machine():
    """Reset after error or E-stop."""
    from main import esp_client
    success = await esp_client.send_command({"type": "reset"})
    return {"success": success}


@router.post("/machine/move")
async def move_machine(x: float = None, y: float = None, z: float = None):
    """Move to absolute position."""
    cmd = {"type": "move_to"}
    if x is not None: cmd["x"] = x
    if y is not None: cmd["y"] = y
    if z is not None: cmd["z"] = z
    from main import esp_client
    success = await esp_client.send_command(cmd)
    return {"success": success}


@router.post("/machine/jog")
async def jog_machine(dx: float = 0, dy: float = 0, dz: float = 0):
    """Relative jog movement."""
    from main import esp_client
    success = await esp_client.send_command({
        "type": "move_rel", "dx": dx, "dy": dy, "dz": dz
    })
    return {"success": success}


@router.post("/claw/open")
async def claw_open():
    """Open the claw."""
    from main import esp_client
    success = await esp_client.send_command({"type": "claw_open"})
    return {"success": success}


@router.post("/claw/close")
async def claw_close(strength: int = 100):
    """Close the claw."""
    from main import esp_client
    success = await esp_client.send_command({"type": "claw_close", "strength": strength})
    return {"success": success}


@router.post("/lights/scene")
async def set_light_scene(scene: str = "idle"):
    """Set the LED lighting scene."""
    from main import esp_client
    success = await esp_client.send_command({"type": "led_scene", "scene": scene})
    return {"success": success, "scene": scene}


@router.post("/lights/brightness")
async def set_brightness(brightness: int = 128):
    """Set LED brightness (0–255)."""
    from main import esp_client
    success = await esp_client.send_command({"type": "led_bright", "brightness": brightness})
    return {"success": success}


@router.post("/motors/enable")
async def enable_motors():
    from main import esp_client
    success = await esp_client.send_command({"type": "enable_motors"})
    return {"success": success}


@router.post("/motors/disable")
async def disable_motors():
    from main import esp_client
    success = await esp_client.send_command({"type": "disable_motors"})
    return {"success": success}


@router.post("/maintenance/enter")
async def enter_maintenance():
    from main import esp_client
    success = await esp_client.send_command({"type": "maintenance", "enter": True})
    return {"success": success}


@router.post("/maintenance/exit")
async def exit_maintenance():
    from main import esp_client
    success = await esp_client.send_command({"type": "maintenance", "enter": False})
    return {"success": success}
