#include "led_controller.h"
#include "config.h"

LedController::LedController(int pinRed, int pinGreen)
    : _pinRed(pinRed), _pinGreen(pinGreen),
      _currentMode(OFF), _lastBlinkTime(0), _blinkState(false) {}

void LedController::begin() {
    pinMode(_pinRed, OUTPUT);
    pinMode(_pinGreen, OUTPUT);
    digitalWrite(_pinRed, LOW);
    digitalWrite(_pinGreen, LOW);
}

void LedController::off() {
    _currentMode = OFF;
    digitalWrite(_pinRed, LOW);
    digitalWrite(_pinGreen, LOW);
}

void LedController::setActive() {
    _currentMode = GREEN;
    digitalWrite(_pinRed, LOW);
    digitalWrite(_pinGreen, HIGH);
}

void LedController::setWarning() {
    _currentMode = RED;
    digitalWrite(_pinGreen, LOW);
    digitalWrite(_pinRed, HIGH);
}

void LedController::setIdle() {
    _currentMode = BOTH;
}

void LedController::setCounting() {
    _currentMode = BOTH;
}

void LedController::setConfig() {
    _currentMode = BOTH;
}

void LedController::update() {
    if (_currentMode != BOTH) return;

    unsigned long now = millis();
    if (now - _lastBlinkTime >= LED_BLINK_INTERVAL) {
        _blinkState = !_blinkState;
        _lastBlinkTime = now;

        // 红绿交替闪烁
        if (_blinkState) {
            digitalWrite(_pinRed, HIGH);
            digitalWrite(_pinGreen, LOW);
        } else {
            digitalWrite(_pinRed, LOW);
            digitalWrite(_pinGreen, HIGH);
        }
    }
}