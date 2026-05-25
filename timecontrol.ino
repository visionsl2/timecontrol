/**
 * ESP32-S3 时间管控器
 *
 * 功能：检测Nintendo Switch是否在盒内，超时报警
 * 硬件：ESP32-S3 + PN532 NFC模块 + 双色LED + 蜂鸣器
 *
 * 作者：Developer
 * 更新：2026-05-23
 */

#include <Arduino.h>
#include <Preferences.h>    // NVS持久化存储
#include <Wire.h>           // I2C通信
#include <WiFi.h>           // WiFi连接
#include <HTTPClient.h>     // HTTP请求
#include <WebServer.h>      // Web服务器
#include <SPIFFS.h>         // 文件系统
#include <Adafruit_PN532.h> // PN532 NFC驱动
#include "html_page.h"      // HTML页面（单独文件避免Arduino预处理器解析JS）

// ==================== 硬件引脚配置 ====================
#define PIN_NFC_SDA    1   // NFC模块I2C数据引脚
#define PIN_NFC_SCL    2   // NFC模块I2C时钟引脚
#define PIN_LED_R     12   // 红色LED引脚
#define PIN_LED_G     13   // 绿色LED引脚
#define PIN_BUZZER    14   // 蜂鸣器引脚（有源蜂鸣器，低电平触发）
#define PIN_BTN        0   // 配网按钮引脚（长按3秒进入配置模式）

// ==================== 时间配置 ====================
#define DEFAULT_TIMEOUT_MS   (30 * 60 * 1000)  // 默认超时30分钟
#define LED_BLINK_INTERVAL    1000              // LED闪烁间隔1秒
#define BTN_LONG_PRESS_MS     3000              // 按钮长按阈值3秒
#define NFC_READ_INTERVAL     500               // NFC读取间隔（防止频繁读取）

// ==================== WiFi AP配置 ====================
#define AP_SSID "SwitchTimeControl"  // 配置模式热点名称
#define AP_PASS "12345678"            // 配置模式热点密码

// ==================== 存储配置 ====================
#define MAX_REGISTERED_TAGS  10       // 最大注册NFC标签数量
#define NVS_NAMESPACE "timecontrol"   // NVS命名空间

// NVS键名定义
#define NVS_KEY_WIFI_SSID   "wifi_ssid"    // WiFi SSID
#define NVS_KEY_WIFI_PASS   "wifi_pass"    // WiFi密码
#define NVS_KEY_TIMEOUT     "timeout_ms"    // 超时时间（毫秒）
#define NVS_KEY_NOTIFY_URL  "notify_url"    // 通知URL
#define NVS_KEY_TAG_COUNT   "tag_count"     // 已注册标签数量
#define NVS_KEY_TAG_PREFIX  "tag_"           // 标签键名前缀

// ==================== 状态枚举 ====================
/**
 * 设备状态机：
 * IDLE    -> 初始状态，等待NFC标签
 * ACTIVE  -> 检测到已注册标签，Switch在盒内
 * COUNTING -> 未检测到标签，开始计时
 * WARNING -> 超时报警
 * CONFIG  -> 配置模式（AP热点）
 */
enum State {
    STATE_IDLE,      // 空闲：红绿交替闪烁
    STATE_ACTIVE,    // 运行：绿灯常亮
    STATE_COUNTING,  // 计时：绿灯闪烁
    STATE_WARNING,   // 警告：红灯闪烁+蜂鸣器响
    STATE_CONFIG     // 配置：红绿交替闪烁+Web服务器
};

// ==================== 全局变量 ====================
Preferences prefs;                    // NVS持久化存储实例
Adafruit_PN532 nfc(255, 255);         // PN532实例（255表示不用IRQ/RST引脚）

// LED状态
bool blinkState = false;              // 闪烁状态
unsigned long lastBlinkTime = 0;      // 上次闪烁时间

// 蜂鸣器状态
bool isAlarming = false;              // 是否报警中
unsigned long alarmStartTime = 0;     // 报警开始时间

// 按钮状态
bool btnPressed = false;              // 按钮是否按下
unsigned long btnPressStart = 0;      // 按钮按下开始时间

// 计时状态
unsigned long startTime = 0;         // 计时开始时间
bool isRunning = false;               // 是否正在计时

// 系统状态
bool configMode = false;             // 是否处于配置模式
WebServer* server = nullptr;          // Web服务器指针
State currentState = STATE_IDLE;      // 当前状态

// ==================== 配置管理函数 ====================
// 函数前向声明（避免编译顺序问题）
bool isTagRegistered(const String& uid);

// 获取/设置WiFi SSID
String getWifiSsid() { return prefs.getString(NVS_KEY_WIFI_SSID, ""); }
void setWifiSsid(const String& s) { prefs.putString(NVS_KEY_WIFI_SSID, s.c_str()); }

// 获取/设置WiFi密码
String getWifiPass() { return prefs.getString(NVS_KEY_WIFI_PASS, ""); }
void setWifiPass(const String& s) { prefs.putString(NVS_KEY_WIFI_PASS, s.c_str()); }

// 获取/设置超时时间（毫秒）
uint32_t getTimeoutMs() { return prefs.getULong(NVS_KEY_TIMEOUT, DEFAULT_TIMEOUT_MS); }
void setTimeoutMs(uint32_t ms) { prefs.putULong(NVS_KEY_TIMEOUT, ms); }

// 获取/设置通知URL
String getNotifyUrl() { return prefs.getString(NVS_KEY_NOTIFY_URL, ""); }
void setNotifyUrl(const String& s) { prefs.putString(NVS_KEY_NOTIFY_URL, s.c_str()); }

// 获取已注册标签数量
uint8_t getTagCount() { return prefs.getUChar(NVS_KEY_TAG_COUNT, 0); }

// 根据索引获取标签UID
String getTag(uint8_t i) {
    if (i >= getTagCount()) return "";
    String key = String(NVS_KEY_TAG_PREFIX) + i;
    return prefs.getString(key.c_str(), "");
}

// 添加标签到白名单
void addTag(const String& uid) {
    if (isTagRegistered(uid)) return;          // 已存在则跳过
    if (getTagCount() >= MAX_REGISTERED_TAGS) return;  // 达到上限则跳过
    String key = String(NVS_KEY_TAG_PREFIX) + getTagCount();
    prefs.putString(key.c_str(), uid.c_str());
    prefs.putUChar(NVS_KEY_TAG_COUNT, getTagCount() + 1);
}

// 从白名单移除标签
void removeTag(uint8_t i) {
    uint8_t cnt = getTagCount();
    if (i >= cnt) return;
    // 将后面的标签向前移动
    for (uint8_t j = i; j < cnt - 1; j++) {
        String k1 = String(NVS_KEY_TAG_PREFIX) + j;
        String k2 = String(NVS_KEY_TAG_PREFIX) + (j + 1);
        prefs.putString(k1.c_str(), prefs.getString(k2.c_str(), "").c_str());
    }
    prefs.remove(String(NVS_KEY_TAG_PREFIX + cnt - 1).c_str());
    prefs.putUChar(NVS_KEY_TAG_COUNT, cnt - 1);
}

// 检查标签是否已注册
bool isTagRegistered(const String& uid) {
    for (uint8_t i = 0; i < getTagCount(); i++) if (getTag(i) == uid) return true;
    return false;
}

// ==================== NFC防抖参数 ====================
#define NFC_LOCK_MS      20000   // 检测成功后锁定20秒不重复读取
#define NFC_CHECK_DELAY  200     // CHECKING状态下每200ms检测一次
#define NFC_CHECK_COUNT  10       // 连续失败次数阈值（确认离开）

// NFC检测状态机
enum NfcDetectState {
    NFC_PRESENT,   // 标签在位（锁定中）
    NFC_CHECKING,   // 检测中（连续检测确认离开）
    NFC_ABSENT     // 确认离开（开始计时）
};
NfcDetectState nfcDetectState = NFC_ABSENT;  // 初始状态：未检测到

// NFC检测相关变量
unsigned long nfcLockUntil = 0;       // 锁定截止时间
unsigned long lastNfcReadTime = 0;   // 上次读取时间
uint8_t nfcFailCount = 0;             // 连续失败计数
String currentUid = "";               // 当前检测到的UID

// ==================== NFC模块 ====================
/**
 * 将二进制UID转换为十六进制字符串
 */
String uidToHex(const uint8_t* uid, uint8_t len) {
    String r = "";
    for (uint8_t i = 0; i < len; i++) {
        if (i > 0) r += ":";
        if (uid[i] < 16) r += "0";
        r += String(uid[i], HEX);
    }
    r.toUpperCase();
    return r;
}

/**
 * 检测NFC标签（带防抖逻辑）
 * @return true 表示NFC标签在位，false 表示标签离开
 *
 * 防抖逻辑：
 * 1. 读取成功 → 锁定20秒，期间直接返回true
 * 2. 20秒后首次读取失败 → 进入CHECKING状态，每200ms检测一次
 * 3. CHECKING期间任何一次成功 → 重置为PRESENT
 * 4. 连续10次失败 → 确认离开，返回false，开始计时
 */
bool isNfcPresent() {
    unsigned long now = millis();

    switch (nfcDetectState) {
        case NFC_PRESENT:
            // 锁定期间：标签被认为一直在位
            if (now < nfcLockUntil) {
                return true;  // 锁定中，直接返回在位
            }
            // 锁定到期，开始检测
            Serial.println("[NFC] Lock expired, checking...");
            nfcDetectState = NFC_CHECKING;
            nfcFailCount = 0;
            // 继续执行下面的检测逻辑
            break;

        case NFC_CHECKING:
            // 每200ms检测一次
            if (now - lastNfcReadTime < NFC_CHECK_DELAY) {
                return true;  // 冷却中，返回上次结果
            }
            lastNfcReadTime = now;

            // 尝试读取NFC标签
            {
                uint8_t uid[10], len;
                if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &len, 100)) {
                    // 读取成功！标签回来了
                    currentUid = uidToHex(uid, len);
                    Serial.println("[NFC] Tag detected during check: " + currentUid);
                    nfcDetectState = NFC_PRESENT;
                    nfcLockUntil = now + NFC_LOCK_MS;
                    nfcFailCount = 0;
                    return true;
                }
            }

            // 读取失败
            nfcFailCount++;
            Serial.println("[NFC] Check " + String(nfcFailCount) + "/" + String(NFC_CHECK_COUNT) + " failed");

            if (nfcFailCount >= NFC_CHECK_COUNT) {
                // 连续10次失败，确认离开
                Serial.println("[NFC] All checks failed, tag is ABSENT");
                nfcDetectState = NFC_ABSENT;
                currentUid = "";
                return false;
            }
            return true;  // 还在检测中，认为暂时在位

        case NFC_ABSENT:
            // 已确认离开，连续检测看是否回来
            if (now - lastNfcReadTime < NFC_CHECK_DELAY) {
                return false;  // 冷却中
            }
            lastNfcReadTime = now;

            {
                uint8_t uid[10], len;
                if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &len, 100)) {
                    // 标签回来了！
                    currentUid = uidToHex(uid, len);
                    Serial.println("[NFC] Tag returned: " + currentUid);
                    nfcDetectState = NFC_PRESENT;
                    nfcLockUntil = now + NFC_LOCK_MS;
                    nfcFailCount = 0;
                    return true;
                }
            }
            return false;  // 仍未检测到
    }

    return false;
}

/**
 * 主动触发一次NFC检测（用于Web界面显示）
 * @return 检测到的UID字符串
 */
String detectNFC() {
    uint8_t uid[10], len;
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &len, 100)) {
        return uidToHex(uid, len);
    }
    return "";
}

// ==================== LED控制 ====================
/**
 * 初始化LED引脚
 */
void ledInit() {
    pinMode(PIN_LED_R, OUTPUT);
    pinMode(PIN_LED_G, OUTPUT);
    digitalWrite(PIN_LED_R, LOW);
    digitalWrite(PIN_LED_G, LOW);
}

/**
 * 根据状态更新LED显示
 * IDLE:    红绿交替闪烁
 * ACTIVE:  绿灯常亮
 * COUNTING: 绿灯闪烁
 * WARNING: 红灯闪烁
 * CONFIG:  红绿交替闪烁
 */
void ledUpdate(State s) {
    unsigned long now = millis();
    // 每秒切换闪烁状态
    if (now - lastBlinkTime >= LED_BLINK_INTERVAL) {
        blinkState = !blinkState;
        lastBlinkTime = now;
    }
    switch (s) {
        case STATE_IDLE:
            // 红绿交替闪烁
            if (blinkState) { digitalWrite(PIN_LED_R, HIGH); digitalWrite(PIN_LED_G, LOW); }
            else { digitalWrite(PIN_LED_R, LOW); digitalWrite(PIN_LED_G, HIGH); }
            break;
        case STATE_ACTIVE:
            // 绿灯常亮
            digitalWrite(PIN_LED_R, LOW);
            digitalWrite(PIN_LED_G, HIGH);
            break;
        case STATE_COUNTING:
            // 绿灯闪烁
            if (blinkState) { digitalWrite(PIN_LED_R, LOW); digitalWrite(PIN_LED_G, HIGH); }
            else { digitalWrite(PIN_LED_R, LOW); digitalWrite(PIN_LED_G, LOW); }
            break;
        case STATE_WARNING:
            // 红灯闪烁
            if (blinkState) { digitalWrite(PIN_LED_R, HIGH); digitalWrite(PIN_LED_G, LOW); }
            else { digitalWrite(PIN_LED_R, LOW); digitalWrite(PIN_LED_G, LOW); }
            break;
        case STATE_CONFIG:
            // 红绿交替闪烁（与IDLE相同）
            if (blinkState) { digitalWrite(PIN_LED_R, HIGH); digitalWrite(PIN_LED_G, LOW); }
            else { digitalWrite(PIN_LED_R, LOW); digitalWrite(PIN_LED_G, HIGH); }
            break;
    }
}

// ==================== 蜂鸣器控制 ====================
/**
 * 初始化蜂鸣器
 */
void buzzerInit() { pinMode(PIN_BUZZER, OUTPUT); digitalWrite(PIN_BUZZER, LOW); }

/**
 * 蜂鸣器响一声
 * @param ms 持续时间（毫秒）
 */
void buzzerBeep(int ms = 100) { digitalWrite(PIN_BUZZER, HIGH); delay(ms); digitalWrite(PIN_BUZZER, LOW); }

/**
 * 更新蜂鸣器状态
 * @note 报警时每500ms切换一次
 */
void buzzerUpdate() {
    if (!isAlarming) return;
    unsigned long t = millis() - alarmStartTime;
    digitalWrite(PIN_BUZZER, (t / 500) % 2 == 0 ? HIGH : LOW);
}

// ==================== Web服务器 ====================
/**
 * 从请求体中解析指定键的值（优化版URL解码，避免内存碎片）
 * @param body 请求体字符串
 * @param key 要查找的键名
 * @return 键对应的值，未找到返回空字符串
 */
String getValue(const String& body, const String& key) {
    int k = body.indexOf(key + "=");
    if (k == -1) return "";
    int v1 = k + key.length() + 1;
    int v2 = body.indexOf("&", v1);
    if (v2 == -1) v2 = body.length();

    // 高效URL解码：一次遍历完成
    String value;
    value.reserve(v2 - v1);  // 预分配确切大小，避免重分配

    for (int i = v1; i < v2; i++) {
        char c = body.charAt(i);
        if (c == '+') {
            value += ' ';
        } else if (c == '%' && i + 2 < v2) {
            // URL解码 %XX
            char h = body.charAt(i + 1);
            char l = body.charAt(i + 2);
            int hi = -1, lo = -1;

            if (h >= '0' && h <= '9') hi = h - '0';
            else if (h >= 'A' && h <= 'F') hi = h - 'A' + 10;
            else if (h >= 'a' && h <= 'f') hi = h - 'a' + 10;

            if (l >= '0' && l <= '9') lo = l - '0';
            else if (l >= 'A' && l <= 'F') lo = l - 'A' + 10;
            else if (l >= 'a' && l <= 'f') lo = l - 'a' + 10;

            if (hi >= 0 && lo >= 0) {
                value += (char)(hi * 16 + lo);
                i += 2;  // 跳过已解码的两个字符
            } else {
                value += c;
            }
        } else {
            value += c;
        }
    }
    return value;
}

/**
 * 初始化Web服务器和路由
 */
void setupWeb() {
    SPIFFS.begin(true);
    server = new WebServer(80);

    // 主页
    server->on("/", HTTP_GET, []() {
        server->send(200, "text/html", HTML_PAGE);
    });

    // 获取配置
    server->on("/api/config", HTTP_GET, []() {
        String json = "{";
        json += "\"timeout\":" + String(getTimeoutMs() / 60000) + ",";
        json += "\"wifi_ssid\":\"" + getWifiSsid() + "\",";
        json += "\"notify_url\":\"" + getNotifyUrl() + "\"";
        json += "}";
        server->send(200, "application/json", json);
    });

    // 保存配置
    server->on("/api/config", HTTP_POST, []() {
        // ESP32 WebServer 对 application/x-www-form-urlencoded
        // 使用 server->arg("name") 直接获取参数值
        String ssid = server->arg("wifi_ssid");
        String pass = server->arg("wifi_pass");
        String to = server->arg("timeout");
        String url = server->arg("notify_url");

        Serial.println("POST /api/config");
        Serial.println("ssid: '" + ssid + "'");
        Serial.println("pass len: " + String(pass.length()));
        Serial.println("to: '" + to + "'");
        Serial.println("url: '" + url + "'");

        if (ssid.length() > 0) {
            setWifiSsid(ssid);
            Serial.println("WiFi SSID saved: " + ssid);
        }
        if (pass.length() > 0) {
            setWifiPass(pass);
            Serial.println("WiFi PASS saved");
        }
        if (to.length() > 0) {
            setTimeoutMs((uint32_t)to.toInt() * 60000);
            Serial.println("Timeout saved: " + to);
        }
        if (url.length() > 0) {
            setNotifyUrl(url);
            Serial.println("URL saved");
        }

        server->send(200, "application/json", "{\"status\":\"ok\",\"reboot\":true}");
        Serial.println("Config saved, rebooting...");
        delay(500);
        ESP.restart();
    });

    // 重启设备
    server->on("/api/reboot", HTTP_POST, []() {
        Serial.println("Rebooting...");
        server->send(200, "application/json", "{\"status\":\"ok\"}");
        delay(100);
        ESP.restart();
    });

    // 获取当前状态
    server->on("/api/status", HTTP_GET, []() {
        String stateStr;
        switch (currentState) {
            case STATE_IDLE: stateStr = "IDLE"; break;
            case STATE_ACTIVE: stateStr = "ACTIVE"; break;
            case STATE_COUNTING: stateStr = "COUNTING"; break;
            case STATE_WARNING: stateStr = "WARNING"; break;
            default: stateStr = "UNKNOWN"; break;
        }

        String elapsedStr = "";
        if (currentState == STATE_COUNTING && isRunning) {
            unsigned long elapsed = millis() - startTime;
            elapsedStr = String(elapsed / 1000);
        }

        String json = "{";
        json += "\"state\":\"" + stateStr + "\",";
        json += "\"nfc\":\"" + currentUid + "\",";
        json += "\"elapsed\":\"" + elapsedStr + "\",";
        json += "\"nfcDetectState\":" + String((int)nfcDetectState) + ",";
        json += "\"configMode\":" + String(configMode ? "true" : "false");
        json += "}";
        server->send(200, "application/json", json);
    });

    // 获取NFC状态（检测到的标签+已注册标签列表）
    server->on("/api/nfc", HTTP_GET, []() {
        // 使用缓存的UID
        String detected = currentUid;
        if (detected.isEmpty()) {
            // 如果缓存为空，尝试主动检测一次
            detected = detectNFC();
        }
        Serial.println("/api/nfc called, detected: " + detected);
        String json = "{";
        json += "\"detected\":\"" + detected + "\",";
        json += "\"tags\":[";
        for (uint8_t i = 0; i < getTagCount(); i++) {
            if (i > 0) json += ",";
            json += "\"" + getTag(i) + "\"";
        }
        json += "]}";
        server->send(200, "application/json", json);
    });

    // 添加标签到白名单
    server->on("/api/nfc/add", HTTP_POST, []() {
        // 尝试多种方式获取POST数据
        String body = server->arg("plain");
        String uid = server->arg("uid");
        Serial.println("POST /api/nfc/add - plain: '" + body + "' uid: '" + uid + "'");

        if (uid.isEmpty()) {
            uid = getValue(body, "uid");
        }
        if (!uid.isEmpty()) {
            addTag(uid);
            Serial.println("Tag added: " + uid);
        } else {
            Serial.println("UID empty, not added");
        }
        server->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // 从白名单移除标签
    server->on("/api/nfc/remove", HTTP_POST, []() {
        String body = server->arg("plain");
        Serial.println("POST /api/nfc/remove body: " + body);
        String idx = getValue(body, "index");
        if (!idx.isEmpty()) {
            removeTag(idx.toInt());
            Serial.println("Tag removed at index: " + idx);
        }
        server->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server->begin();
    Serial.println("Web server started");
}

// ==================== 主程序 ====================
/**
 * 进入配置模式
 * - 启动AP热点
 * - 启动Web服务器
 * - 切换到CONFIG状态
 */
void enterConfigMode() {
    configMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);

    // 清理旧服务器
    if (server != nullptr) {
        server->stop();
        delete server;
        server = nullptr;
    }

    setupWeb();
    currentState = STATE_CONFIG;
    buzzerBeep(200);
    Serial.println("Config mode started");
    Serial.println("AP IP: " + WiFi.softAPIP().toString());
}

/**
 * 初始化程序
 */
void setup() {
    Serial.begin(115200);
    delay(500);

    // 初始化NVS存储
    prefs.begin(NVS_NAMESPACE, false);

    // 初始化硬件
    ledInit();
    buzzerInit();
    pinMode(PIN_BTN, INPUT_PULLUP);

    // 初始化I2C和NFC模块
    Wire.begin(PIN_NFC_SDA, PIN_NFC_SCL);
    Wire.setClock(400000);  // 400kHz快速模式
    Serial.println("I2C已初始化");

    delay(100);  // 等待PN532完全启动
    nfc.begin();
    Serial.println("PN532.begin()完成");

    // 检查NFC模块
    uint32_t v = nfc.getFirmwareVersion();
    if (v) {
        Serial.print("PN532 v"); Serial.print((v >> 16) & 0xFF); Serial.print("."); Serial.println((v >> 8) & 0xFF);
        nfc.SAMConfig();
        Serial.println("NFC ready");
    } else {
        Serial.println("PN532 not found");
    }

    // 检查WiFi配置
    String ssid = getWifiSsid();
    if (ssid.length() > 0) {
        // 有配置，尝试连接
        WiFi.begin(ssid.c_str(), getWifiPass().c_str());
        unsigned long t = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t < 10000) delay(100);
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("WiFi: " + WiFi.localIP().toString());
            // WiFi连接成功后启动Web服务器
            setupWeb();
            configMode = false;
        } else {
            Serial.println("WiFi failed, entering config mode");
            enterConfigMode();
        }
    } else {
        // 无配置，直接进入配置模式
        Serial.println("No WiFi config, entering config mode");
        enterConfigMode();
    }

    Serial.println("System ready");
}

/**
 * 主循环 - 优化版NFC检测
 */
void loop() {
    // 处理Web请求
    server->handleClient();

    // 检测按钮长按（进入配置模式）
    bool longPress = false;
    bool btn = (digitalRead(PIN_BTN) == LOW);
    if (btn && !btnPressed) {
        btnPressed = true;
        btnPressStart = millis();
    } else if (!btn && btnPressed) {
        btnPressed = false;
    }
    if (btnPressed && millis() - btnPressStart >= BTN_LONG_PRESS_MS) {
        longPress = true;
        btnPressed = false;
    }

    if (longPress) {
        enterConfigMode();
    }

    // 配置模式下：LED闪烁
    if (configMode) {
        ledUpdate(currentState);
        buzzerUpdate();
        delay(50);
        return;
    }

    // ========== 正常工作模式 ==========

    // 使用防抖逻辑检测NFC
    bool nfcPresent = isNfcPresent();
    bool registered = nfcPresent && isTagRegistered(currentUid);

    // 调试：每2秒打印一次状态
    static unsigned long lastDebugTime = 0;
    if (millis() - lastDebugTime > 2000) {
        Serial.print("[Monitor] NFC: ");
        Serial.print(currentUid.length() > 0 ? currentUid : "(none)");
        Serial.print(" | NFC State: ");
        switch (nfcDetectState) {
            case NFC_PRESENT: Serial.print("PRESENT"); break;
            case NFC_CHECKING: Serial.print("CHECKING"); break;
            case NFC_ABSENT: Serial.print("ABSENT"); break;
        }
        Serial.print(" | System State: ");
        switch (currentState) {
            case STATE_IDLE: Serial.print("IDLE"); break;
            case STATE_ACTIVE: Serial.print("ACTIVE"); break;
            case STATE_COUNTING: Serial.print("COUNTING"); break;
            case STATE_WARNING: Serial.print("WARNING"); break;
            default: Serial.print("UNKNOWN"); break;
        }
        Serial.print(" | Registered: ");
        Serial.println(registered ? "YES" : "NO");
        lastDebugTime = millis();
    }

    // 状态机逻辑
    switch (currentState) {
        case STATE_IDLE:
            // 等待已注册标签出现
            if (registered) {
                Serial.println(">>> Switch detected, entering ACTIVE state");
                currentState = STATE_ACTIVE;
                isRunning = false;
                buzzerBeep(100);
            }
            break;

        case STATE_ACTIVE:
            // 标签仍在，保持ACTIVE
            if (!nfcPresent) {
                // 标签消失，开始计时
                Serial.println(">>> Switch removed, starting COUNTDOWN");
                currentState = STATE_COUNTING;
                startTime = millis();
                isRunning = true;
            }
            break;

        case STATE_COUNTING:
            if (nfcPresent) {
                // 标签恢复，停止计时
                Serial.println(">>> Switch returned, back to ACTIVE");
                currentState = STATE_ACTIVE;
                isRunning = false;
            } else if (millis() - startTime >= getTimeoutMs()) {
                // 超时，进入警告状态
                Serial.println(">>> TIMEOUT! Starting WARNING");
                currentState = STATE_WARNING;
                isAlarming = true;
                alarmStartTime = millis();
            }
            break;

        case STATE_WARNING:
            if (nfcPresent) {
                // 标签恢复，退出警告
                Serial.println(">>> Switch returned, clearing WARNING");
                currentState = STATE_IDLE;
                isRunning = false;
                isAlarming = false;
                buzzerBeep(200);
            }
            break;

        default:
            currentState = STATE_IDLE;
            break;
    }

    // 更新LED和蜂鸣器
    ledUpdate(currentState);
    buzzerUpdate();
    delay(50);
}