#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>

class LedController {
public:
    LedController(int pinRed, int pinGreen);

    void begin();
    void setIdle();      // 红绿交替闪烁
    void setActive();    // 绿灯常亮
    void setCounting();  // 绿灯闪烁
    void setWarning();   // 红灯闪烁
    void setConfig();     // 红绿交替闪烁
    void off();

    void update();  // 在loop中调用更新闪烁

private:
    int _pinRed;
    int _pinGreen;
    enum Mode { OFF, RED, GREEN, BOTH } _currentMode;
    unsigned long _lastBlinkTime;
    bool _blinkState;
};

#endif