#include "button_manager.h"

ButtonManager::ButtonManager(int pin)
    : _pin(pin), _pressStartTime(0), _isPressed(false) {}

void ButtonManager::begin() {
    pinMode(_pin, INPUT_PULLUP);
}

bool ButtonManager::checkLongPress(unsigned long holdTimeMs) {
    bool currentState = (digitalRead(_pin) == LOW);

    if (currentState && !_isPressed) {
        // 按下开始
        _isPressed = true;
        _pressStartTime = millis();
    } else if (!currentState && _isPressed) {
        // 松开，重置
        _isPressed = false;
    }

    if (_isPressed && (millis() - _pressStartTime >= holdTimeMs)) {
        _isPressed = false;  // 重置，只触发一次
        return true;
    }

    return false;
}