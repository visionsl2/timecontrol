#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

#include <Arduino.h>

class ConfigManager;

class TimerManager {
public:
    TimerManager();

    void begin();
    void setConfig(ConfigManager* config);
    void startCounting();
    void reset();
    bool isTimeout();
    unsigned long getElapsedMs();

private:
    ConfigManager* _config;
    unsigned long _startTime;
    bool _isRunning;
    uint32_t _timeoutMs;
};

#endif