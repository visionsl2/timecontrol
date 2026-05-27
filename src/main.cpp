/**
 * NFC模块测试程序
 *
 * 功能：测试NFC独立模块的正确性
 * 1. 初始化串口，输出日志
 * 2. setup()中读取一次NFC检查标签是否存在
 * 3. loop()中检测状态变化，有变化则输出
 * 4. NFC中断事件防抖：延迟50ms读取，再延迟2000ms防止频繁中断
 */

#include <Arduino.h>
#include "nfc_module.h"

// ==================== 全局变量 ====================
NfcModule nfc;                      // NFC模块实例
bool lastNfcState = true;           // 上次NFC状态
String lastUid = "";                // 上次UID
bool initialized = false;           // 初始化标志

// ==================== NFC回调函数 ====================
void onNfcEvent(bool present, String uid) {
    // 可以在这里添加其他处理逻辑
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
    Serial.println("[Init] NFC Module Test Started");
    Serial.println("========================================");

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
    } else {
        Serial.println("[Setup] No NFC detected, assuming present");
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
        // 状态变化，输出日志
        if (currentState) {
            Serial.println("[NFC] State changed: PRESENT, UID: " + currentUid);
        } else {
            Serial.println("[NFC] State changed: ABSENT");
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