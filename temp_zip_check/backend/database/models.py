"""
Database Models — SQLAlchemy + SQLite
"""
from sqlalchemy import Column, Integer, String, Float, Boolean, DateTime, Text
from sqlalchemy.ext.asyncio import create_async_engine, AsyncSession, async_sessionmaker
from sqlalchemy.orm import DeclarativeBase
from datetime import datetime, timezone
from pathlib import Path
from config.settings import settings


class Base(DeclarativeBase):
    pass


class Game(Base):
    """Record of a single game play."""
    __tablename__ = "games"

    id = Column(Integer, primary_key=True, autoincrement=True)
    timestamp = Column(DateTime, default=lambda: datetime.now(timezone.utc), nullable=False)
    player_name = Column(String(100), default="Player")
    duration_seconds = Column(Integer, default=0)
    result = Column(String(20), default="unknown")  # win, loss, abandoned
    prize_detected = Column(Boolean, default=False)
    score = Column(Integer, default=0)
    grab_x = Column(Float, nullable=True)
    grab_y = Column(Float, nullable=True)
    claw_strength = Column(Integer, nullable=True)


class Player(Base):
    """Player leaderboard entry."""
    __tablename__ = "players"

    id = Column(Integer, primary_key=True, autoincrement=True)
    name = Column(String(100), unique=True, nullable=False)
    games_played = Column(Integer, default=0)
    games_won = Column(Integer, default=0)
    total_score = Column(Integer, default=0)
    last_played = Column(DateTime, nullable=True)


class MachineConfig(Base):
    """Persistent machine configuration storage."""
    __tablename__ = "machine_config"

    key = Column(String(100), primary_key=True)
    value = Column(Text, nullable=False)
    updated_at = Column(DateTime, default=lambda: datetime.now(timezone.utc),
                        onupdate=lambda: datetime.now(timezone.utc))


class ErrorLog(Base):
    """Machine error log."""
    __tablename__ = "error_log"

    id = Column(Integer, primary_key=True, autoincrement=True)
    timestamp = Column(DateTime, default=lambda: datetime.now(timezone.utc))
    code = Column(Integer, nullable=False)
    message = Column(String(500), nullable=False)
    severity = Column(String(20), default="ERROR")


# Database engine setup
def _get_db_path() -> str:
    db_dir = Path(settings.db_path).parent
    db_dir.mkdir(parents=True, exist_ok=True)
    return f"sqlite+aiosqlite:///{settings.db_path}"


engine = create_async_engine(_get_db_path(), echo=settings.debug)
async_session = async_sessionmaker(engine, class_=AsyncSession, expire_on_commit=False)


async def init_db():
    """Create all tables if they don't exist."""
    async with engine.begin() as conn:
        await conn.run_sync(Base.metadata.create_all)


async def get_session() -> AsyncSession:
    """Get a database session for dependency injection."""
    async with async_session() as session:
        yield session
