/**
 * 蜂鸣器控制模块实现
 */

#include "buzzer_module.h"

// ==================== 构造函数 ====================
BuzzerController::BuzzerController() {
}

// ==================== 初始化 ====================
void BuzzerController::begin() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);  // 默认关闭蜂鸣器（高电平）
    Serial.println("[Buzzer] Initialized");
}

// ==================== 响指定毫秒 ====================
void BuzzerController::beep(int ms) {
    digitalWrite(BUZZER_PIN, HIGH);   // 低电平触发
    delay(ms);
    digitalWrite(BUZZER_PIN, LOW);  // 高电平关闭
}

// ==================== 告警提示 ====================
void BuzzerController::alert() {
    beep(500);
}