/**
 * 蜂鸣器控制模块
 *
 * 功能：控制有源蜂鸣器
 * 触发条件：NFC状态变化时蜂鸣器响
 */

#ifndef BUZZER_MODULE_H
#define BUZZER_MODULE_H

#include <Arduino.h>

// ==================== 常量定义 ====================
#define BUZZER_PIN   14      // 蜂鸣器引脚（有源蜂鸣器，低电平触发）

// ==================== 蜂鸣器控制类 ====================
class BuzzerController {
public:
    BuzzerController();

    // 初始化
    void begin();

    // 响指定毫秒
    void beep(int ms);

    // 告警提示（500ms）
    void alert();
};

#endif // BUZZER_MODULE_H