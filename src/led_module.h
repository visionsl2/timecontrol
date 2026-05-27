/**
 * LED控制模块
 *
 * 功能：控制红绿双色LED
 * 绿色LED：NFC在位时亮
 * 红色LED：NFC离开时亮
 */

#ifndef LED_MODULE_H
#define LED_MODULE_H

#include <Arduino.h>

// ==================== 常量定义 ====================
#define LED_G_PIN   13      // 绿色LED引脚
#define LED_R_PIN   12      // 红色LED引脚

// ==================== LED控制类 ====================
class LedController {
private:
    bool _greenOn;
    bool _redOn;

public:
    LedController();

    // 初始化
    void begin();

    // 设置绿色LED
    void setGreen(bool on);

    // 设置红色LED
    void setRed(bool on);

    // NFC在位：绿色亮
    void setPresent();

    // NFC离开：红色亮
    void setAbsent();

    // 关闭所有LED
    void off();
};

#endif // LED_MODULE_H