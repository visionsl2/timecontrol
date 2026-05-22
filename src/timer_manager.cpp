#include "timer_manager.h"
#include "config_manager.h"
#include "config.h"

TimerManager::TimerManager()
    : _config(nullptr), _startTime(0), _isRunning(false), _timeoutMs(DEFAULT_TIMEOUT_MS) {
}

void TimerManager::begin() {
    _startTime = 0;
    _isRunning = false;
}

void TimerManager::setConfig(ConfigManager* config) {
    _config = config;
    if (config != nullptr) {
        _timeoutMs = config->getTimeoutMs();
    }
}

void TimerManager::startCounting() {
    _startTime = millis();
    _isRunning = true;
}

void TimerManager::reset() {
    _startTime = 0;
    _isRunning = false;
}

bool TimerManager::isTimeout() {
    if (!_isRunning) {
        return false;
    }
    return getElapsedMs() >= _timeoutMs;
}

unsigned long TimerManager::getElapsedMs() {
    if (!_isRunning) {
        return 0;
    }
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - _startTime;

    // Handle millis() overflow (occurs every ~49.7 days)
    if (currentTime < _startTime) {
        elapsed = (ULONG_MAX - _startTime) + currentTime;
    }

    return elapsed;
}