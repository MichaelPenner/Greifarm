/**
 * State Manager — Reactive state store for the UI
 */
export class StateManager {
    constructor() {
        this.state = {
            connected: false,
            espConnected: false,
            machineState: 'UNKNOWN',
            position: { x: 0, y: 0, z: 0 },
            homed: false,
            moving: false,
            claw: 'UNKNOWN',
            clawAngle: 0,
            joystick: { x: 0, y: 0, z: 0, button: false },
            sensors: 0,
            estop: false,
            prizeBeam: false,
            credits: 0,
            gameTime: 0,
            gamesPlayed: 0,
            gamesWon: 0,
            safety: 0,
            motorsOn: false,
            uptime: 0,
            ledScene: 'off',
            errors: [],
        };
        this.listeners = new Map();
    }

    get(key) {
        return this.state[key];
    }

    update(updates) {
        const changed = {};
        for (const [key, value] of Object.entries(updates)) {
            if (JSON.stringify(this.state[key]) !== JSON.stringify(value)) {
                this.state[key] = value;
                changed[key] = value;
            }
        }

        if (Object.keys(changed).length > 0) {
            this._emit('change', changed);
            for (const key of Object.keys(changed)) {
                this._emit(`change:${key}`, changed[key]);
            }
        }
    }

    updateFromTelemetry(data) {
        this.update({
            machineState: data.state || 'UNKNOWN',
            position: { x: data.x || 0, y: data.y || 0, z: data.z || 0 },
            homed: data.homed || false,
            moving: data.moving || false,
            claw: data.claw || 0,
            clawAngle: data.claw_angle || 0,
            joystick: {
                x: data.joy_x || 0,
                y: data.joy_y || 0,
                z: data.joy_z || 0,
                button: data.joy_btn || false,
            },
            sensors: data.sensors || 0,
            estop: data.estop || false,
            prizeBeam: data.prize_beam || false,
            credits: data.credits || 0,
            gameTime: data.game_time || 0,
            gamesPlayed: data.games_played || 0,
            gamesWon: data.games_won || 0,
            safety: data.safety || 0,
            motorsOn: data.motors_on || false,
            uptime: data.uptime || 0,
        });
    }

    on(event, callback) {
        if (!this.listeners.has(event)) {
            this.listeners.set(event, []);
        }
        this.listeners.get(event).push(callback);
    }

    _emit(event, data) {
        const cbs = this.listeners.get(event);
        if (cbs) cbs.forEach(cb => cb(data));
    }

    // Derived state helpers
    get winRate() {
        return this.state.gamesPlayed > 0
            ? (this.state.gamesWon / this.state.gamesPlayed * 100).toFixed(1)
            : '0.0';
    }

    get isPlaying() {
        return ['PLAYING', 'CLAW_DESCENDING', 'CLAW_GRABBING',
                'CLAW_RETRACTING', 'RETURNING', 'RELEASING',
                'CHECKING_PRIZE'].includes(this.state.machineState);
    }

    get isReady() {
        return this.state.machineState === 'READY';
    }

    get isError() {
        return this.state.machineState === 'ERROR' ||
               this.state.machineState === 'EMERGENCY_STOP';
    }

    get gameTimeFormatted() {
        const seconds = Math.ceil(this.state.gameTime / 1000);
        return `${seconds}s`;
    }

    get uptimeFormatted() {
        const s = Math.floor(this.state.uptime / 1000);
        const h = Math.floor(s / 3600);
        const m = Math.floor((s % 3600) / 60);
        return `${h}h ${m}m`;
    }

    get clawStateString() {
        const states = ['UNKNOWN', 'OPEN', 'CLOSED', 'PARTIAL', 'MOVING'];
        return states[this.state.claw] || 'UNKNOWN';
    }
}
