/**
 * NFC + LED + 蜂鸣器测试程序
 *
 * 功能：测试NFC独立模块 + LED + 蜂鸣器
 * 1. 初始化串口，输出日志
 * 2. setup()中读取一次NFC检查标签是否存在
 * 3. loop()中检测状态变化，更新LED和蜂鸣器
 * 4. NFC中断事件防抖：延迟50ms读取，再延迟1000ms防止频繁中断
 */

#include <Arduino.h>
#include "nfc_module.h"
#include "led_module.h"
#include "buzzer_module.h"

// ==================== 全局变量 ====================
NfcModule nfc;                      // NFC模块实例
LedController led;                   // LED模块实例
BuzzerController buzzer;              // 蜂鸣器模块实例
bool lastNfcState = true;            // 上次NFC状态
String lastUid = "";                 // 上次UID
bool initialized = false;          // 初始化标志

// ==================== NFC回调函数 ====================
void onNfcEvent(bool present, String uid) {
    // 根据NFC状态更新LED
    if (present) {
        led.setPresent();
    } else {
        led.setAbsent();
    }

    Serial.println("[Callback] NFC state changed: " + String(present ? "PRESENT" : "ABSENT") +
                   ", UID: " + (uid.length() > 0 ? uid : "(none)"));
}

// ==================== setup ====================
void setup() {
    // 初始化串口
    Serial.begin(115200);
    delay(500);
    Serial.println();
    Serial.println("========================================");
    Serial.println("[Init] NFC + LED + Buzzer Test Started");
    Serial.println("========================================");

    // 初始化LED模块
    led.begin();

    // 初始化蜂鸣器模块
    buzzer.begin();

    // 初始化NFC模块
    if (!nfc.begin()) {
        Serial.println("[E] NFC module initialization failed!");
        return;
    }

    // 设置回调
    nfc.onEvent(onNfcEvent);

    // setup中主动读取一次NFC
    Serial.println("[Setup] Checking NFC...");
    String uid = "";
    if (nfc.readTag(uid)) {
        Serial.println("[Setup] NFC detected: " + uid);
        lastUid = uid;
        led.setPresent();  // NFC存在，绿色亮
    } else {
        Serial.println("[Setup] No NFC detected, assuming absent");
        led.setAbsent();   // NFC不存在，红色亮
    }

    lastNfcState = nfc.isPresent();
    initialized = true;

    Serial.println("[Setup] Initialization complete");
    Serial.println();
}

// ==================== loop ====================
void loop() {
    if (!initialized) return;

    // 处理NFC中断
    nfc.processInterrupt();

    // 检测状态变化
    bool currentState = nfc.isPresent();
    String currentUid = nfc.getLastUid();

    if (currentState != lastNfcState) {
        // 状态变化，更新LED和蜂鸣器
        if (currentState) {
            Serial.println("[NFC] State changed: PRESENT, UID: " + currentUid);
            led.setPresent();
            buzzer.beep(500);  // NFC放回，蜂鸣器响500ms
        } else {
            Serial.println("[NFC] State changed: ABSENT");
            led.setAbsent();
        }
        lastNfcState = currentState;
    }

    // 每5秒输出当前状态（监控）
    static unsigned long lastMonitorTime = 0;
    if (millis() - lastMonitorTime >= 5000) {
        lastMonitorTime = millis();
        Serial.print("[Monitor] State: ");
        Serial.print(currentState ? "PRESENT" : "ABSENT");
        Serial.print(", UID: ");
        Serial.println(currentUid.length() > 0 ? currentUid : "(none)");
    }

    delay(50);
}