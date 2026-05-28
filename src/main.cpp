/**
 * NFC + LED + 蜂鸣器 + WiFi测试程序
 *
 * 功能：测试NFC独立模块 + LED + 蜂鸣器 + WiFi配网
 * 1. WiFi连接或配网模式
 * 2. NTP对时获取当前时间
 * 3. NFC检测状态变化，更新LED和蜂鸣器
 */

#include <Arduino.h>
#include <Preferences.h>
#include <time.h>
#include "nfc_module.h"
#include "led_module.h"
#include "buzzer_module.h"
#include "wifi_module.h"

// ==================== 常量定义 ====================
#define TIMEOUT_MS           (30 * 60 * 1000)  // 30分钟
#define ALERT_INTERVAL_MS    500               // 报警闪烁间隔
#define NTP_SYNC_THRESHOLD   1704067200L       // 2024-01-01 00:00:00
#define TIMER_SAVE_INTERVAL   1000              // 计时器保存间隔（1秒）

// ==================== 全局变量 ====================
NfcModule nfc;                      // NFC模块实例
LedController led;                   // LED模块实例
BuzzerController buzzer;              // 蜂鸣器模块实例
WiFiManager wifi;                    // WiFi模块实例
bool lastNfcState = true;            // 上次NFC状态
String lastUid = "";                 // 上次UID
String registeredUid = "";            // 已注册的NFC UID
bool initialized = false;            // 初始化标志

// 计时器相关
unsigned long absentDuration = 0;    // 累计离开时长（毫秒）
unsigned long absentStart = 0;       // 本次离开开始时间
int lastDayCode = 0;                 // 记录当天的日期编码
bool isWarning = false;             // 报警状态
unsigned long lastAlertTime = 0;    // 上次报警时间
unsigned long lastTimerSaveTime = 0; // 上次保存计时器时间
Preferences timerPrefs;             // 计时器NVS

// ==================== NFC验证LED更新 ====================
void updateLedWithValidation(bool nfcPresent, String currentUid) {
    if (registeredUid.length() == 0) {
        // 未注册：不作UID校验，保持原有行为
        if (nfcPresent) {
            led.setPresent();
        } else {
            led.setAbsent();
        }
    } else {
        // 已注册：验证UID是否匹配
        if (nfcPresent && currentUid == registeredUid) {
            led.setPresent();  // 绿灯：正确的NFC标签
        } else {
            led.setAbsent();   // 红灯：标签不匹配或离开
        }
    }
}

// ==================== 计时器辅助函数 ====================

// 安全计算时间差（处理millis溢出）
unsigned long safeMillisDiff(unsigned long now, unsigned long past) {
    if (now >= past) {
        return now - past;
    } else {
        // 溢出情况
        return (ULONG_MAX - past) + now + 1;
    }
}

// 检查NTP是否已同步
bool isNtpSynced() {
    time_t now = time(nullptr);
    return now > NTP_SYNC_THRESHOLD;
}

// 检查跨天
bool isNewDay() {
    bool crossed = false;

    // 方案1: NTP已同步，使用正常日期
    if (isNtpSynced()) {
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        int today = (t->tm_year + 1900) * 10000 + (t->tm_mon + 1) * 100 + t->tm_mday;
        if (lastDayCode > 0 && today != lastDayCode) {
            crossed = true;
        }
        lastDayCode = today;
        timerPrefs.putInt("last_day", today);
    }

    // 方案2: NTP未同步，使用millis()天数作为备用
    unsigned long currentDayMillis = millis() / 86400000UL;
    unsigned long lastDayMillis = timerPrefs.getULong("last_millis_day", 0);
    if (lastDayMillis > 0 && currentDayMillis != lastDayMillis) {
        crossed = true;
    }
    timerPrefs.putULong("last_millis_day", currentDayMillis);

    if (crossed) {
        absentDuration = 0;
        absentStart = 0;
        Serial.println("[Timer] Cross-day reset");
    }
    return crossed;
}

// 恢复计时器状态
void restoreTimer() {
    lastDayCode = timerPrefs.getInt("last_day", 0);
    absentDuration = timerPrefs.getULong("absent_duration", 0);
    absentStart = timerPrefs.getULong("absent_start", 0);

    // Sanity check: 异常值检测和修复
    // 检查所有值是否合理
    // 1. 如果 absenStart 有值，检查是否在合理范围内（应该是过去某个时间点）
    //    异常情况：absentStart 很小但不是0，说明可能被损坏（读到了高位数据）
    // 2. 如果累计时长 > TIMEOUT_MS，说明值异常
    bool corrupt = false;

    // 检查 absentStart 是否异常（非0但很小，可能是高位字节损坏）
    if (absentStart > 0 && absentStart < 10000) {
        // absentStart 应该是很久以前的时间（约50天前的值）
        // 如果 < 10秒，说明absentStart损坏了（应该是 ULONG_MAX - XXXXX）
        corrupt = true;
    }

    // 检查累计时长是否异常
    if (absentDuration > TIMEOUT_MS) {
        corrupt = true;
    }

    if (corrupt) {
        // 检测到异常值（NVS损坏或高位字节丢失），重置计时器
        absentDuration = 0;
        absentStart = 0;
        lastDayCode = 0;
        timerPrefs.putULong("absent_duration", 0);
        timerPrefs.putULong("absent_start", 0);
        timerPrefs.putInt("last_day", 0);
    }


    // 检查跨天
    if (isNewDay()) {
        absentDuration = 0;
        absentStart = 0;
    } else if (absentStart > 0) {
        // 上次在计时中，恢复并累加时间
        unsigned long elapsed = safeMillisDiff(millis(), absentStart);
        absentDuration += elapsed;
        absentStart = millis();
    }
}

// 获取当前累计离开时长
unsigned long getCurrentAbsentDuration() {
    if (absentStart > 0) {
        return absentDuration + safeMillisDiff(millis(), absentStart);
    }
    return absentDuration;
}

// 处理NFC离开
void handleNfcAbsent() {
    if (absentStart == 0) {
        absentStart = millis();
        timerPrefs.putULong("absent_start", absentStart);
        Serial.println("[Timer] Absent started");
    }
}

// 处理NFC返回
void handleNfcPresent() {
    if (absentStart > 0) {
        unsigned long elapsed = safeMillisDiff(millis(), absentStart);
        absentDuration += elapsed;
        absentStart = 0;
        timerPrefs.putULong("absent_duration", absentDuration);
        timerPrefs.putULong("absent_start", 0);
        Serial.println("[Timer] Absent stopped, duration: " + String(absentDuration) + "ms");
    }
}

// 处理报警
void handleAlert() {
    unsigned long now = millis();

    // LED闪烁控制（200ms间隔闪烁）
    static bool redState = false;
    static unsigned long lastRedBlink = 0;
    if (now - lastRedBlink >= 200) {
        lastRedBlink = now;
        redState = !redState;
        led.setRed(redState);
    }

    // 蜂鸣器急促报警（每500ms发出一串短促蜂鸣）
    if (now - lastAlertTime >= ALERT_INTERVAL_MS) {
        lastAlertTime = now;
        // 发出急促的3声短促蜂鸣
        buzzer.beep(50);
        delay(60);
        buzzer.beep(50);
        delay(60);
        buzzer.beep(50);
    }
}

// 停止报警
void stopAlert() {
    isWarning = false;
    // 恢复正常的LED状态
    bool currentState = nfc.isPresent();
    String currentUid = nfc.getLastUid();
    updateLedWithValidation(currentState, currentUid);
}

// 判断是否应该继续计时
bool shouldTimerRun(bool nfcPresent, String currentUid) {
    if (!nfcPresent) {
        return true;  // NFC离开，计时
    }

    // NFC在位
    if (registeredUid.length() == 0) {
        // 未注册UID，任何NFC都停止计时
        return false;
    }

    // 有注册UID，检查是否匹配
    return currentUid != registeredUid;  // 不匹配继续计时，匹配停止计时
}

// ==================== NFC回调函数 ====================
void onNfcEvent(bool present, String uid) {
    updateLedWithValidation(present, uid);

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
    Serial.println("[Init] NFC + LED + Buzzer + WiFi Test");
    Serial.println("========================================");

    // 初始化LED模块
    led.begin();

    // 初始化WiFi模块
    wifi.setLedController(&led);
    wifi.setButtonPin(0);  // GPIO0
    wifi.setNfcModule(&nfc);

    if (!wifi.begin()) {
        // 无配置，已自动进入配网模式
        Serial.println("[WiFi] Config mode active");
    } else {
        // 有配置，尝试连接
        int retries = 0;
        while (retries < WIFI_MAX_RETRIES) {
            Serial.println("[WiFi] Connection attempt " + String(retries + 1) + "/" + String(WIFI_MAX_RETRIES));
            if (wifi.connect()) {
                Serial.println("[WiFi] Connected successfully");
                break;
            }
            retries++;
        }

        if (retries >= WIFI_MAX_RETRIES || !wifi.isConnected()) {
            Serial.println("[WiFi] Connection failed, entering config mode");
            wifi.enterConfigMode();
        }
    }

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
        updateLedWithValidation(true, uid);
    } else {
        Serial.println("[Setup] No NFC detected, assuming absent");
        updateLedWithValidation(false, "");
    }

    lastNfcState = nfc.isPresent();
    initialized = true;

    // 读取已注册的NFC UID并打印
    {
        Preferences prefs;
        prefs.begin("wifi_cfg", true);
        registeredUid = prefs.getString("nfc_uid", "");
        if (registeredUid.length() > 0) {
            Serial.println("[Setup] UID validation ENABLED, registered: " + registeredUid);
        } else {
            Serial.println("[Setup] UID validation DISABLED (no registered UID)");
        }
    }

    // 初始化计时器NVS并恢复状态
    timerPrefs.begin("timer", false);
    restoreTimer();
    Serial.println("[Setup] Timer: duration=" + String(absentDuration) + "ms, start=" + String(absentStart));

    Serial.println("[Setup] Initialization complete");
    Serial.println();
}

// ==================== loop ====================
void loop() {
    if (!initialized) return;

    // 处理WiFi（配网模式）
    wifi.handle();

    // 处理NFC中断
    nfc.processInterrupt();

    // 检测状态变化
    bool currentState = nfc.isPresent();
    String currentUid = nfc.getLastUid();

    if (currentState != lastNfcState) {
        // 状态变化，更新LED（带UID验证）
        updateLedWithValidation(currentState, currentUid);

        if (currentState) {
            Serial.println("[NFC] State changed: PRESENT, UID: " + currentUid);
            buzzer.beep(500);  // NFC放回，蜂鸣器响500ms
            handleNfcPresent();  // 停止计时
            stopAlert();         // 停止报警
        } else {
            Serial.println("[NFC] State changed: ABSENT");
            handleNfcAbsent();   // 开始计时
        }
        lastNfcState = currentState;
    } else if (currentState && currentUid != lastUid) {
        // NFC存在但UID变化了，更新LED
        updateLedWithValidation(currentState, currentUid);
        lastUid = currentUid;
    }

    // 计时器逻辑
    if (shouldTimerRun(currentState, currentUid)) {
        // 应该计时
        if (absentStart == 0) {
            handleNfcAbsent();
        }

        unsigned long currentDuration = getCurrentAbsentDuration();

        // 定期保存计时器状态
        if (millis() - lastTimerSaveTime >= TIMER_SAVE_INTERVAL) {
            lastTimerSaveTime = millis();
            timerPrefs.putULong("absent_duration", currentDuration);
        }

        // 检查是否超时报警
        if (currentDuration >= TIMEOUT_MS) {
            isWarning = true;
            handleAlert();
        }

        // 报警状态：主动读取NFC（移到条件外，每次都尝试）
        String alertUid = "";
        if (nfc.readTag(alertUid)) {
            Serial.print("[AlertCheck] NFC read: ");
            Serial.println(alertUid);
            if (registeredUid.length() > 0 && alertUid == registeredUid) {
                handleNfcPresent();
                stopAlert();
            }
        }
    } else {
        // 不应该计时，停止计时
        if (absentStart > 0) {
            handleNfcPresent();
            stopAlert();
        }
    }

    // 跨天检测
    isNewDay();

    // 每5秒输出当前状态（监控）
    static unsigned long lastMonitorTime = 0;
    if (millis() - lastMonitorTime >= 5000) {
        lastMonitorTime = millis();
        unsigned long dur = getCurrentAbsentDuration();
        Serial.print("[Monitor] WiFi: ");
        Serial.print(wifi.isConfigMode() ? "CONFIG" : (wifi.isConnected() ? "CONNECTED" : "DISCONNECTED"));
        Serial.print(", IP: ");
        Serial.print(wifi.getLocalIP());
        Serial.print(" | NFC: ");
        Serial.print(currentState ? "PRESENT" : "ABSENT");
        Serial.print(" | Duration: ");
        Serial.print(dur / 1000 / 60);
        Serial.print("min | Warning: ");
        Serial.println(isWarning ? "YES" : "NO");
    }

    delay(50);
}