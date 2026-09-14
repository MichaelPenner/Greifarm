"""
API Routes — Statistics & Leaderboard
"""
from fastapi import APIRouter, Depends
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select, func, desc
from database.models import Game, Player, get_session

router = APIRouter()


@router.get("/statistics")
async def get_statistics(session: AsyncSession = Depends(get_session)):
    """Get overall machine statistics."""
    total_games = (await session.execute(
        select(func.count(Game.id))
    )).scalar() or 0

    total_wins = (await session.execute(
        select(func.count(Game.id)).where(Game.result == "win")
    )).scalar() or 0

    total_prizes = (await session.execute(
        select(func.count(Game.id)).where(Game.prize_detected == True)
    )).scalar() or 0

    avg_duration = (await session.execute(
        select(func.avg(Game.duration_seconds))
    )).scalar() or 0

    win_rate = (total_wins / total_games * 100) if total_games > 0 else 0

    return {
        "games_played": total_games,
        "games_won": total_wins,
        "total_prizes": total_prizes,
        "win_rate": round(win_rate, 1),
        "avg_game_duration": round(float(avg_duration), 1),
    }


@router.get("/leaderboard")
async def get_leaderboard(
    limit: int = 10,
    session: AsyncSession = Depends(get_session)
):
    """Get player leaderboard sorted by wins."""
    stmt = (
        select(Player)
        .order_by(desc(Player.games_won), desc(Player.total_score))
        .limit(limit)
    )
    result = await session.execute(stmt)
    players = result.scalars().all()

    return [
        {
            "rank": i + 1,
            "name": p.name,
            "games_played": p.games_played,
            "games_won": p.games_won,
            "win_rate": round(p.games_won / p.games_played * 100, 1) if p.games_played > 0 else 0,
            "total_score": p.total_score,
        }
        for i, p in enumerate(players)
    ]


@router.get("/games/recent")
async def get_recent_games(
    limit: int = 20,
    session: AsyncSession = Depends(get_session)
):
    """Get recent game history."""
    stmt = select(Game).order_by(desc(Game.timestamp)).limit(limit)
    result = await session.execute(stmt)
    games = result.scalars().all()

    return [
        {
            "id": g.id,
            "timestamp": g.timestamp.isoformat() if g.timestamp else None,
            "player": g.player_name,
            "result": g.result,
            "duration": g.duration_seconds,
            "prize": g.prize_detected,
        }
        for g in games
    ]
