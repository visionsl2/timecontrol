#ifndef BUZZER_CONTROLLER_H
#define BUZZER_CONTROLLER_H

#include <Arduino.h>

class BuzzerController {
public:
    BuzzerController(int pin);

    void begin();
    void beep(int durationMs = 100);
    void startAlarm();
    void stop();

    void update();  // 在loop中调用

private:
    int _pin;
    bool _isAlarming;
    unsigned long _alarmStartTime;
    unsigned long _lastBeepTime;
    bool _beepState;
};

#endif