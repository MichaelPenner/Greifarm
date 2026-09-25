/**
 * Audio Manager — Sound playback with browser autoplay handling
 */
export class AudioManager {
    constructor(soundsPath = '/sounds') {
        this.soundsPath = soundsPath;
        this.sounds = new Map();
        this.enabled = true;
        this.volume = 0.7;
        this.unlocked = false;

        // Sound event mapping
        this.eventMap = {
            'game_start':     'game_start.mp3',
            'countdown':      'countdown.mp3',
            'claw_close':     'claw_close.mp3',
            'prize_detected': 'win.mp3',
            'win':            'win.mp3',
            'loss':           'loss.mp3',
            'error':          'error.mp3',
            'button_click':   'click.mp3',
            'coin_insert':    'coin_insert.wav',
        };
    }

    async init() {
        // Preload all sounds
        for (const [event, file] of Object.entries(this.eventMap)) {
            try {
                const audio = new Audio(`${this.soundsPath}/${file}`);
                audio.preload = 'auto';
                audio.volume = this.volume;
                this.sounds.set(event, audio);
            } catch (e) {
                console.warn(`[Audio] Failed to load: ${file}`);
            }
        }

        // Handle browser autoplay restriction
        // Audio must be unlocked by a user gesture
        const unlock = () => {
            if (this.unlocked) return;
            const silent = new Audio('data:audio/wav;base64,UklGRiQAAABXQVZFZm10IBAAAAABAAEARKwAAIhYAQACABAAZGF0YQAAAAA=');
            silent.play().then(() => {
                this.unlocked = true;
                console.log('[Audio] Unlocked');
            }).catch(() => {});
        };

        document.addEventListener('click', unlock, { once: false });
        document.addEventListener('touchstart', unlock, { once: false });
    }

    play(event) {
        if (!this.enabled || !this.unlocked) return;

        const audio = this.sounds.get(event);
        if (audio) {
            // Clone and play to allow overlapping sounds
            const clone = audio.cloneNode();
            clone.volume = this.volume;
            clone.play().catch(() => {});
        }
    }

    setVolume(vol) {
        this.volume = Math.max(0, Math.min(1, vol));
        this.sounds.forEach(audio => audio.volume = this.volume);
    }

    setEnabled(enabled) {
        this.enabled = enabled;
    }
}
