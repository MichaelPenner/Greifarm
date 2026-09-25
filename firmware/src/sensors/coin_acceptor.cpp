#include "coin_acceptor.h"
#include "../utils/logger.h"

// Define static members
volatile int CoinAcceptor::_coinCount = 0;
volatile unsigned long CoinAcceptor::_lastPulseTime = 0;

CoinAcceptor::CoinAcceptor(uint8_t pin) : _pin(pin) {}

void CoinAcceptor::begin() {
    // Configure pin with internal pullup
    // When the coin acceptor pulses, it will pull this pin to GND
    pinMode(_pin, INPUT_PULLUP);
    
    // Attach interrupt for FALLING edge (HIGH to LOW)
    attachInterrupt(digitalPinToInterrupt(_pin), _isr, FALLING);
    
    LOG_I("COIN", "Coin acceptor initialized on pin %d", _pin);
}

int CoinAcceptor::getPendingCoins() {
    if (_coinCount > 0) {
        // Disable interrupts briefly to read and clear safely
        noInterrupts();
        int count = _coinCount;
        _coinCount = 0;
        interrupts();
        return count;
    }
    return 0;
}

// Interrupt Service Routine
void IRAM_ATTR CoinAcceptor::_isr() {
    unsigned long now = millis();
    // Debounce: ignore pulses closer than 50ms (adjust based on coin acceptor speed)
    if (now - _lastPulseTime > 50) {
        _coinCount++;
        _lastPulseTime = now;
    }
}
