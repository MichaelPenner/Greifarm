#pragma once
// =============================================================
// Coin Acceptor — Pulse counter via Interrupt
// =============================================================

#ifndef COIN_ACCEPTOR_H
#define COIN_ACCEPTOR_H

#include <Arduino.h>

class CoinAcceptor {
public:
    CoinAcceptor(uint8_t pin);

    void begin();
    
    // Get the number of coins inserted since last call
    int getPendingCoins();

private:
    uint8_t _pin;
    static volatile int _coinCount;
    static volatile unsigned long _lastPulseTime;

    static void IRAM_ATTR _isr();
};

#endif // COIN_ACCEPTOR_H
