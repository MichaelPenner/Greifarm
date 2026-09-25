/**
 * WebSocket Client — Persistent connection to backend
 */
export class WebSocketClient {
    constructor(url) {
        this.url = url || `ws://${location.host}/ws`;
        this.ws = null;
        this.connected = false;
        this.listeners = new Map();
        this.reconnectInterval = 3000;
        this.heartbeatInterval = 2000;
        this._heartbeatTimer = null;
        this._reconnectTimer = null;
    }

    connect() {
        try {
            this.ws = new WebSocket(this.url);

            this.ws.onopen = () => {
                console.log('[WS] Connected');
                this.connected = true;
                this._emit('connection', { connected: true });
                this._startHeartbeat();
            };

            this.ws.onclose = () => {
                console.log('[WS] Disconnected');
                this.connected = false;
                this._emit('connection', { connected: false });
                this._stopHeartbeat();
                this._scheduleReconnect();
            };

            this.ws.onerror = (err) => {
                console.error('[WS] Error:', err);
            };

            this.ws.onmessage = (event) => {
                try {
                    const data = JSON.parse(event.data);
                    this._emit(data.type, data);
                    this._emit('message', data);
                } catch (e) {
                    console.warn('[WS] Invalid message:', event.data);
                }
            };
        } catch (e) {
            console.error('[WS] Connection failed:', e);
            this._scheduleReconnect();
        }
    }

    send(data) {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify(data));
            return true;
        }
        return false;
    }

    sendCommand(type, params = {}) {
        return this.send({ type, ...params });
    }

    on(event, callback) {
        if (!this.listeners.has(event)) {
            this.listeners.set(event, []);
        }
        this.listeners.get(event).push(callback);
    }

    off(event, callback) {
        const cbs = this.listeners.get(event);
        if (cbs) {
            this.listeners.set(event, cbs.filter(cb => cb !== callback));
        }
    }

    _emit(event, data) {
        const cbs = this.listeners.get(event);
        if (cbs) {
            cbs.forEach(cb => {
                try { cb(data); } catch (e) { console.error('[WS] Listener error:', e); }
            });
        }
    }

    _startHeartbeat() {
        this._heartbeatTimer = setInterval(() => {
            this.send({ type: 'heartbeat' });
        }, this.heartbeatInterval);
    }

    _stopHeartbeat() {
        if (this._heartbeatTimer) {
            clearInterval(this._heartbeatTimer);
            this._heartbeatTimer = null;
        }
    }

    _scheduleReconnect() {
        if (this._reconnectTimer) return;
        this._reconnectTimer = setTimeout(() => {
            this._reconnectTimer = null;
            console.log('[WS] Reconnecting...');
            this.connect();
        }, this.reconnectInterval);
    }

    disconnect() {
        this._stopHeartbeat();
        if (this._reconnectTimer) {
            clearTimeout(this._reconnectTimer);
            this._reconnectTimer = null;
        }
        if (this.ws) {
            this.ws.close();
        }
    }
}
