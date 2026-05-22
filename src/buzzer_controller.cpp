#include "buzzer_controller.h"

BuzzerController::BuzzerController(int pin)
    : _pin(pin), _isAlarming(false), _alarmStartTime(0),
      _lastBeepTime(0), _beepState(false) {}

void BuzzerController::begin() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
}

void BuzzerController::beep(int durationMs) {
    digitalWrite(_pin, HIGH);
    delay(durationMs);
    digitalWrite(_pin, LOW);
}

void BuzzerController::startAlarm() {
    _isAlarming = true;
    _alarmStartTime = millis();
    _lastBeepTime = 0;
}

void BuzzerController::stop() {
    _isAlarming = false;
    digitalWrite(_pin, LOW);
}

void BuzzerController::update() {
    if (!_isAlarming) return;

    unsigned long now = millis();
    // 响0.5秒，停0.5秒
    unsigned long cycleTime = now - _alarmStartTime;
    bool shouldBeOn = (cycleTime / 500) % 2 == 0;

    if (shouldBeOn) {
        digitalWrite(_pin, HIGH);
    } else {
        digitalWrite(_pin, LOW);
    }
}