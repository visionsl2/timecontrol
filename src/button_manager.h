#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>

class ButtonManager {
public:
    ButtonManager(int pin);

    void begin();
    bool checkLongPress(unsigned long holdTimeMs);

private:
    int _pin;
    unsigned long _pressStartTime;
    bool _isPressed;
};

#endif