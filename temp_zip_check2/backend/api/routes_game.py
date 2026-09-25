"""
API Routes — Game Control
"""
from fastapi import APIRouter, Depends
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select, func
from datetime import datetime, timezone
from database.models import Game, Player, get_session

router = APIRouter()


@router.post("/game/start")
async def start_game(player: str = "Player", session: AsyncSession = Depends(get_session)):
    """Start a new game. The actual game start command is sent via WebSocket."""
    # This endpoint is for the REST API; the WebSocket command is the primary method
    from main import esp_client
    success = await esp_client.send_command({
        "type": "game_start",
        "player": player,
    })
    return {"success": success, "player": player}


@router.post("/game/stop")
async def stop_game():
    """Stop the current game."""
    from main import esp_client
    success = await esp_client.send_command({"type": "game_stop"})
    return {"success": success}


@router.post("/game/record")
async def record_game(
    player_name: str = "Player",
    result: str = "loss",
    duration: int = 0,
    prize_detected: bool = False,
    grab_x: float = None,
    grab_y: float = None,
    session: AsyncSession = Depends(get_session)
):
    """Record a completed game in the database."""
    game = Game(
        player_name=player_name,
        result=result,
        duration_seconds=duration,
        prize_detected=prize_detected,
        grab_x=grab_x,
        grab_y=grab_y,
    )
    session.add(game)

    # Update player record
    stmt = select(Player).where(Player.name == player_name)
    existing = (await session.execute(stmt)).scalar_one_or_none()

    if existing:
        existing.games_played += 1
        if result == "win":
            existing.games_won += 1
        existing.last_played = datetime.now(timezone.utc)
    else:
        player = Player(
            name=player_name,
            games_played=1,
            games_won=1 if result == "win" else 0,
            last_played=datetime.now(timezone.utc),
        )
        session.add(player)

    await session.commit()
    return {"success": True, "game_id": game.id}


@router.post("/credits/add")
async def add_credits(amount: int = 1):
    """Add credits to the machine."""
    from main import esp_client
    # Credits are tracked on the ESP, not the backend
    # The backend just forwards the command
    # For now, we use a game event
    return {"success": True, "credits_added": amount}
