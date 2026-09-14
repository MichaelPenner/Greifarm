#pragma once
// =============================================================
// Logger — Structured Logging Utility
// =============================================================

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "../config/machine_config.h"

enum class LogLevel : uint8_t {
    NONE  = 0,
    ERR   = 1,
    WARN  = 2,
    INFO  = 3,
    DEBUG = 4
};

struct LogEntry {
    unsigned long timestamp;
    LogLevel level;
    char module[16];
    char message[128];
};

class Logger {
public:
    static void setLevel(LogLevel level);
    static LogLevel getLevel();

    static void error(const char* module, const char* fmt, ...);
    static void warn(const char* module, const char* fmt, ...);
    static void info(const char* module, const char* fmt, ...);
    static void debug(const char* module, const char* fmt, ...);

    // Access recent log entries for the web UI
    static const LogEntry* getBuffer();
    static int getBufferCount();
    static void clearBuffer();

private:
    static void _log(LogLevel level, const char* module, const char* fmt, va_list args);
    static void _addToBuffer(LogLevel level, const char* module, const char* message);

    static LogLevel _level;
    static LogEntry _buffer[LOG_BUFFER_SIZE];
    static int _bufferHead;
    static int _bufferCount;
};

// Convenience macros
#define LOG_E(module, fmt, ...) Logger::error(module, fmt, ##__VA_ARGS__)
#define LOG_W(module, fmt, ...) Logger::warn(module, fmt, ##__VA_ARGS__)
#define LOG_I(module, fmt, ...) Logger::info(module, fmt, ##__VA_ARGS__)
#define LOG_D(module, fmt, ...) Logger::debug(module, fmt, ##__VA_ARGS__)

#endif // LOGGER_H
