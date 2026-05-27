/**
 * LED控制模块实现
 */

#include "led_module.h"

// ==================== 构造函数 ====================
LedController::LedController() {
    _greenOn = false;
    _redOn = false;
}

// ==================== 初始化 ====================
void LedController::begin() {
    pinMode(LED_G_PIN, OUTPUT);
    pinMode(LED_R_PIN, OUTPUT);
    digitalWrite(LED_G_PIN, LOW);
    digitalWrite(LED_R_PIN, LOW);
    Serial.println("[LED] Initialized");
}

// ==================== 设置绿色LED ====================
void LedController::setGreen(bool on) {
    _greenOn = on;
    digitalWrite(LED_G_PIN, on ? HIGH : LOW);
}

// ==================== 设置红色LED ====================
void LedController::setRed(bool on) {
    _redOn = on;
    digitalWrite(LED_R_PIN, on ? HIGH : LOW);
}

// ==================== NFC在位 ====================
void LedController::setPresent() {
    setRed(false);
    setGreen(true);
    Serial.println("[LED] Green ON (NFC present)");
}

// ==================== NFC离开 ====================
void LedController::setAbsent() {
    setGreen(false);
    setRed(true);
    Serial.println("[LED] Red ON (NFC absent)");
}

// ==================== 关闭所有LED ====================
void LedController::off() {
    setGreen(false);
    setRed(false);
}