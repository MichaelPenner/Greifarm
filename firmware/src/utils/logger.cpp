#include "logger.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>

LogLevel Logger::_level = static_cast<LogLevel>(LOG_LEVEL_DEFAULT);
LogEntry Logger::_buffer[LOG_BUFFER_SIZE];
int Logger::_bufferHead = 0;
int Logger::_bufferCount = 0;

void Logger::setLevel(LogLevel level) { _level = level; }
LogLevel Logger::getLevel() { return _level; }

void Logger::error(const char* module, const char* fmt, ...) {
    if (_level < LogLevel::ERR) return;
    va_list args; va_start(args, fmt); _log(LogLevel::ERR, module, fmt, args); va_end(args);
}

void Logger::warn(const char* module, const char* fmt, ...) {
    if (_level < LogLevel::WARN) return;
    va_list args; va_start(args, fmt); _log(LogLevel::WARN, module, fmt, args); va_end(args);
}

void Logger::info(const char* module, const char* fmt, ...) {
    if (_level < LogLevel::INFO) return;
    va_list args; va_start(args, fmt); _log(LogLevel::INFO, module, fmt, args); va_end(args);
}

void Logger::debug(const char* module, const char* fmt, ...) {
    if (_level < LogLevel::DEBUG) return;
    va_list args; va_start(args, fmt); _log(LogLevel::DEBUG, module, fmt, args); va_end(args);
}

void Logger::_log(LogLevel level, const char* module, const char* fmt, va_list args) {
    char message[128];
    vsnprintf(message, sizeof(message), fmt, args);

    const char* levelStr = "???";
    switch (level) {
        case LogLevel::ERR:  levelStr = "ERR"; break;
        case LogLevel::WARN: levelStr = "WRN"; break;
        case LogLevel::INFO: levelStr = "INF"; break;
        case LogLevel::DEBUG:levelStr = "DBG"; break;
        default: break;
    }

#ifndef NATIVE_BUILD
    Serial.printf("[%lu][%s][%s] %s\n", millis(), levelStr, module, message);
#else
    printf("[%s][%s] %s\n", levelStr, module, message);
#endif

    _addToBuffer(level, module, message);
}

void Logger::_addToBuffer(LogLevel level, const char* module, const char* message) {
    LogEntry& entry = _buffer[_bufferHead];
#ifndef NATIVE_BUILD
    entry.timestamp = millis();
#else
    entry.timestamp = 0;
#endif
    entry.level = level;
    strncpy(entry.module, module, sizeof(entry.module) - 1);
    entry.module[sizeof(entry.module) - 1] = '\0';
    strncpy(entry.message, message, sizeof(entry.message) - 1);
    entry.message[sizeof(entry.message) - 1] = '\0';

    _bufferHead = (_bufferHead + 1) % LOG_BUFFER_SIZE;
    if (_bufferCount < LOG_BUFFER_SIZE) _bufferCount++;
}

const LogEntry* Logger::getBuffer() { return _buffer; }
int Logger::getBufferCount() { return _bufferCount; }
void Logger::clearBuffer() { _bufferHead = 0; _bufferCount = 0; }
