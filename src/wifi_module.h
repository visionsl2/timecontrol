/**
 * WiFi配网模块（可复用）
 *
 * 功能：WiFi连接、配网页面、NTP对时
 * 触发条件：无配置、连接失败3次
 */

#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// ==================== 常量定义 ====================
#define WIFI_AP_SSID      "TimeControl"  // 配网热点名称
#define WIFI_AP_PASS      "12345678"      // 配网热点密码
#define WIFI_MAX_RETRIES   3              // 最大重试次数
#define WIFI_TIMEOUT_MS    15000          // 连接超时15秒
#define WIFI_NAMESPACE     "wifi_cfg"      // NVS命名空间

// ==================== 前向声明 ====================
class LedController;
class NfcModule;

// ==================== WiFi管理类 ====================
class WiFiManager {
private:
    Preferences _prefs;
    WebServer* _server;
    bool _configMode;
    bool _connected;
    LedController* _led;
    NfcModule* _nfc;
    unsigned long _lastBlinkTime;
    bool _blinkState;

    // 按键相关
    int _buttonPin;
    bool _enterConfigRequested;
    bool _saveNfcUidRequested;
    static WiFiManager* _instance;
    static unsigned long _buttonPressStart;
    static void buttonISR();

    void setupWebServer();
    String getConfigHtml();

public:
    WiFiManager();

    // 初始化
    bool begin();

    // 连接WiFi（返回是否连接成功）
    bool connect();

    // 是否已连接
    bool isConnected();

    // 是否配网模式
    bool isConfigMode();

    // 进入配网模式
    void enterConfigMode();

    // 处理（loop中调用）
    void handle();

    // 设置LED控制器（用于配网模式闪烁）
    void setLedController(LedController* led);

    // 设置按键引脚（用于长按3秒进入配网模式）
    void setButtonPin(int pin);

    // 设置NFC模块（用于短按注册UID）
    void setNfcModule(NfcModule* nfc);

    // 获取本机IP
    String getLocalIP();

    // NTP对时
    void syncTime();
};

#endif // WIFI_MODULE_H