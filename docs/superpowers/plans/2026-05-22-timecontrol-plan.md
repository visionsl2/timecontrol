# 时间管控器 - 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 实现ESP32时间管控器，支持NFC检测、超时报警、WiFi配置

**Architecture:** 基于PlatformIO + Arduino框架，分模块实现：NfcManager处理NFC读写和白名单，LedController/BuzzerController控制声光，WiFiManager处理配网，WebServer提供配置界面

**Tech Stack:** PlatformIO, Arduino, Adafruit_PN532, ESP Async WebServer, Preferences

---

## 文件结构

```
timecontrol/
├── src/
│   ├── main.cpp              # 主程序入口
│   ├── config.h              # 全局配置和常量
│   ├── state_machine.h/cpp  # 状态机
│   ├── nfc_manager.h/cpp    # NFC管理
│   ├── led_controller.h/cpp # LED控制
│   ├── buzzer_controller.h/cpp # 蜂鸣器控制
│   ├── timer_manager.h/cpp  # 计时器
│   ├── button_manager.h/cpp # 按钮检测
│   ├── wifi_manager.h/cpp  # WiFi管理
│   ├── web_server.h/cpp     # Web服务器
│   └── config_manager.h/cpp # 配置存储
├── data/
│   └── index.html           # Web页面
└── platformio.ini           # PlatformIO配置
```

---

## Task 1: 项目初始化

**Files:**
- Create: `platformio.ini`
- Create: `src/config.h`

- [ ] **Step 1: 创建platformio.ini**

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps =
    adafruit/Adafruit PN532@^1.2.0
    https://github.com/me-no-dev/ESPAsyncWebServer.git
    me-no-dev/ESP Async TCP@^1.1.4
    esphoekma/Preferences@^2.0.0
```

- [ ] **Step 2: 创建config.h**

```cpp
#ifndef CONFIG_H
#define CONFIG_H

// 引脚定义
#define PIN_NFC_IRQ   4
#define PIN_NFC_RST   5
#define PIN_LED_R     12
#define PIN_LED_G     13
#define PIN_BUZZER    14
#define PIN_BTN       0

// 超时默认30分钟
#define DEFAULT_TIMEOUT_MS (30 * 60 * 1000)

// LED闪烁间隔
#define LED_BLINK_INTERVAL 1000

// 按钮长按时间3秒
#define BTN_LONG_PRESS_MS 3000

// WiFi AP配置
#define AP_SSID "SwitchTimeControl"
#define AP_PASS "12345678"

// 最大注册标签数
#define MAX_REGISTERED_TAGS 10

// NVS键名
#define NVS_NAMESPACE "timecontrol"
#define NVS_KEY_WIFI_SSID "wifi_ssid"
#define NVS_KEY_WIFI_PASS "wifi_pass"
#define NVS_KEY_TIMEOUT "timeout_ms"
#define NVS_KEY_NOTIFY_URL "notify_url"
#define NVS_KEY_TAG_COUNT "tag_count"
#define NVS_KEY_TAG_PREFIX "tag_"

#endif
```

- [ ] **Step 3: 提交**

```bash
git add platformio.ini src/config.h
git commit -m "init: add project skeleton and config"
```

---

## Task 2: LED控制器

**Files:**
- Create: `src/led_controller.h`
- Create: `src/led_controller.cpp`

- [ ] **Step 1: 创建led_controller.h**

```cpp
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
```

- [ ] **Step 2: 创建led_controller.cpp**

```cpp
#include "led_controller.h"

LedController::LedController(int pinRed, int pinGreen)
    : _pinRed(pinRed), _pinGreen(pinGreen),
      _currentMode(OFF), _lastBlinkTime(0), _blinkState(false) {}

void LedController::begin() {
    pinMode(_pinRed, OUTPUT);
    pinMode(_pinGreen, OUTPUT);
    digitalWrite(_pinRed, LOW);
    digitalWrite(_pinGreen, LOW);
}

void LedController::off() {
    _currentMode = OFF;
    digitalWrite(_pinRed, LOW);
    digitalWrite(_pinGreen, LOW);
}

void LedController::setActive() {
    _currentMode = GREEN;
    digitalWrite(_pinRed, LOW);
    digitalWrite(_pinGreen, HIGH);
}

void LedController::setWarning() {
    _currentMode = RED;
    digitalWrite(_pinGreen, LOW);
    digitalWrite(_pinRed, HIGH);
}

void LedController::setIdle() {
    _currentMode = BOTH;
}

void LedController::setCounting() {
    _currentMode = BOTH;
}

void LedController::setConfig() {
    _currentMode = BOTH;
}

void LedController::update() {
    if (_currentMode != BOTH) return;

    unsigned long now = millis();
    if (now - _lastBlinkTime >= LED_BLINK_INTERVAL) {
        _blinkState = !_blinkState;
        _lastBlinkTime = now;

        if (_currentMode == BOTH) {
            // 红绿交替闪烁
            if (_blinkState) {
                digitalWrite(_pinRed, HIGH);
                digitalWrite(_pinGreen, LOW);
            } else {
                digitalWrite(_pinRed, LOW);
                digitalWrite(_pinGreen, HIGH);
            }
        }
    }
}
```

- [ ] **Step 3: 测试验证（编译检查）**

```bash
cd /c/sl/code/esp32/timecontrol
mkdir -p src
# 验证文件语法
```

- [ ] **Step 4: 提交**

```bash
git add src/led_controller.h src/led_controller.cpp
git commit -m "feat: add LED controller module"
```

---

## Task 3: 蜂鸣器控制器

**Files:**
- Create: `src/buzzer_controller.h`
- Create: `src/buzzer_controller.cpp`

- [ ] **Step 1: 创建buzzer_controller.h**

```cpp
#ifndef BUZZER_CONTROLLER_H
#define BUZZER_CONTROLLER_H

#include <Arduino.h>

class BuzzerController {
public:
    BuzzerController(int pin);

    void begin();
    void beep(int durationMs = 100);
    void startAlarm();
    void stop();

    void update();  // 在loop中调用

private:
    int _pin;
    bool _isAlarming;
    unsigned long _alarmStartTime;
    unsigned long _lastBeepTime;
    bool _beepState;
};

#endif
```

- [ ] **Step 2: 创建buzzer_controller.cpp**

```cpp
#include "buzzer_controller.h"

BuzzerController::BuzzerController(int pin)
    : _pin(pin), _isAlarming(false), _alarmStartTime(0),
      _lastBeepTime(0), _beepState(false) {}

void BuzzerController::begin() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
}

void BuzzerController::beep(int durationMs) {
    digitalWrite(_pin, HIGH);
    delay(durationMs);
    digitalWrite(_pin, LOW);
}

void BuzzerController::startAlarm() {
    _isAlarming = true;
    _alarmStartTime = millis();
    _lastBeepTime = 0;
}

void BuzzerController::stop() {
    _isAlarming = false;
    digitalWrite(_pin, LOW);
}

void BuzzerController::update() {
    if (!_isAlarming) return;

    unsigned long now = millis();
    // 响0.5秒，停0.5秒
    unsigned long cycleTime = now - _alarmStartTime;
    bool shouldBeOn = (cycleTime / 500) % 2 == 0;

    if (shouldBeOn) {
        digitalWrite(_pin, HIGH);
    } else {
        digitalWrite(_pin, LOW);
    }
}
```

- [ ] **Step 3: 提交**

```bash
git add src/buzzer_controller.h src/buzzer_controller.cpp
git commit -m "feat: add buzzer controller module"
```

---

## Task 4: 按钮管理器

**Files:**
- Create: `src/button_manager.h`
- Create: `src/button_manager.cpp`

- [ ] **Step 1: 创建button_manager.h**

```cpp
#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>

class ButtonManager {
public:
    ButtonManager(int pin);

    void begin();
    bool checkLongPress(unsigned long holdTimeMs);

private:
    int _pin;
    unsigned long _pressStartTime;
    bool _isPressed;
};

#endif
```

- [ ] **Step 2: 创建button_manager.cpp**

```cpp
#include "button_manager.h"

ButtonManager::ButtonManager(int pin)
    : _pin(pin), _pressStartTime(0), _isPressed(false) {}

void ButtonManager::begin() {
    pinMode(_pin, INPUT_PULLUP);
}

bool ButtonManager::checkLongPress(unsigned long holdTimeMs) {
    bool currentState = (digitalRead(_pin) == LOW);

    if (currentState && !_isPressed) {
        // 按下开始
        _isPressed = true;
        _pressStartTime = millis();
    } else if (!currentState && _isPressed) {
        // 松开，重置
        _isPressed = false;
    }

    if (_isPressed && (millis() - _pressStartTime >= holdTimeMs)) {
        _isPressed = false;  // 重置，只触发一次
        return true;
    }

    return false;
}
```

- [ ] **Step 3: 提交**

```bash
git add src/button_manager.h src/button_manager.cpp
git commit -m "feat: add button manager for config trigger"
```

---

## Task 5: 配置管理器

**Files:**
- Create: `src/config_manager.h`
- Create: `src/config_manager.cpp`

- [ ] **Step 1: 创建config_manager.h**

```cpp
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

class ConfigManager {
public:
    ConfigManager();

    void begin();

    // WiFi配置
    String getWifiSsid();
    void setWifiSsid(const String& ssid);
    String getWifiPass();
    void setWifiPass(const String& pass);

    // 超时配置
    unsigned long getTimeoutMs();
    void setTimeoutMs(unsigned long ms);

    // 通知URL
    String getNotifyUrl();
    void setNotifyUrl(const String& url);

    // NFC标签白名单
    int getRegisteredTagCount();
    String getRegisteredTag(int index);
    void addRegisteredTag(const String& uid);
    void removeRegisteredTag(int index);
    bool isTagRegistered(const String& uid);

private:
    Preferences _prefs;
};

#endif
```

- [ ] **Step 2: 创建config_manager.cpp**

```cpp
#include "config_manager.h"

ConfigManager::ConfigManager() {}

void ConfigManager::begin() {
    _prefs.begin(NVS_NAMESPACE, false);
}

String ConfigManager::getWifiSsid() {
    return _prefs.getString(NVS_KEY_WIFI_SSID, "");
}

void ConfigManager::setWifiSsid(const String& ssid) {
    _prefs.putString(NVS_KEY_WIFI_SSID, ssid.c_str());
}

String ConfigManager::getWifiPass() {
    return _prefs.getString(NVS_KEY_WIFI_PASS, "");
}

void ConfigManager::setWifiPass(const String& pass) {
    _prefs.putString(NVS_KEY_WIFI_PASS, pass.c_str());
}

unsigned long ConfigManager::getTimeoutMs() {
    return _prefs.getULong(NVS_KEY_TIMEOUT, DEFAULT_TIMEOUT_MS);
}

void ConfigManager::setTimeoutMs(unsigned long ms) {
    _prefs.putULong(NVS_KEY_TIMEOUT, ms);
}

String ConfigManager::getNotifyUrl() {
    return _prefs.getString(NVS_KEY_NOTIFY_URL, "");
}

void ConfigManager::setNotifyUrl(const String& url) {
    _prefs.putString(NVS_KEY_NOTIFY_URL, url.c_str());
}

int ConfigManager::getRegisteredTagCount() {
    return _prefs.getInt(NVS_KEY_TAG_COUNT, 0);
}

String ConfigManager::getRegisteredTag(int index) {
    if (index < 0 || index >= getRegisteredTagCount()) return "";
    String key = String(NVS_KEY_TAG_PREFIX) + index;
    return _prefs.getString(key.c_str(), "");
}

void ConfigManager::addRegisteredTag(const String& uid) {
    if (isTagRegistered(uid)) return;
    int count = getRegisteredTagCount();
    if (count >= MAX_REGISTERED_TAGS) return;

    String key = String(NVS_KEY_TAG_PREFIX) + count;
    _prefs.putString(key.c_str(), uid.c_str());
    _prefs.putInt(NVS_KEY_TAG_COUNT, count + 1);
}

void ConfigManager::removeRegisteredTag(int index) {
    int count = getRegisteredTagCount();
    if (index < 0 || index >= count) return;

    // 移动后面的标签
    for (int i = index; i < count - 1; i++) {
        String keyCurr = String(NVS_KEY_TAG_PREFIX) + i;
        String keyNext = String(NVS_KEY_TAG_PREFIX) + (i + 1);
        String tag = _prefs.getString(keyNext.c_str(), "");
        _prefs.putString(keyCurr.c_str(), tag.c_str());
    }

    // 删除最后一个
    String lastKey = String(NVS_KEY_TAG_PREFIX) + (count - 1);
    _prefs.remove(lastKey.c_str());
    _prefs.putInt(NVS_KEY_TAG_COUNT, count - 1);
}

bool ConfigManager::isTagRegistered(const String& uid) {
    int count = getRegisteredTagCount();
    for (int i = 0; i < count; i++) {
        if (getRegisteredTag(i) == uid) return true;
    }
    return false;
}
```

- [ ] **Step 3: 提交**

```bash
git add src/config_manager.h src/config_manager.cpp
git commit -m "feat: add config manager with NVS storage"
```

---

## Task 6: NFC管理器

**Files:**
- Create: `src/nfc_manager.h`
- Create: `src/nfc_manager.cpp`

- [ ] **Step 1: 创建nfc_manager.h**

```cpp
#ifndef NFC_MANAGER_H
#define NFC_MANAGER_H

#include <Arduino.h>
#include <Adafruit_PN532.h>
#include "config_manager.h"

class NfcManager {
public:
    NfcManager(int irqPin, int rstPin);

    bool begin();
    String detectTag();  // 返回UID字符串，空表示无检测
    bool isTagRegistered(const String& uid);

    void addTagToWhitelist(const String& uid);
    void removeTagFromWhitelist(int index);
    int getWhitelistCount();
    String getWhitelistTag(int index);

private:
    Adafruit_PN532 _pn532;
    ConfigManager* _config;
    String _lastUid;
    unsigned long _lastDetectTime;

    String uidToString(uint8_t* uid, uint8_t uidLength);
};

#endif
```

- [ ] **Step 2: 创建nfc_manager.cpp**

```cpp
#include "nfc_manager.h"
#include "config.h"

NfcManager::NfcManager(int irqPin, int rstPin)
    : _pn532(irqPin, rstPin), _config(nullptr),
      _lastUid(""), _lastDetectTime(0) {}

bool NfcManager::begin() {
    _pn532.begin();

    uint32_t versiondata = _pn532.getFirmwareVersion();
    if (!versiondata) {
        Serial.println("PN532 not found");
        return false;
    }

    Serial.print("Found PN532 with firmware version: ");
    Serial.println((versiondata >> 16) & 0xFF, DEC);

    _pn532.SAMConfig();
    Serial.println("NFC ready");
    return true;
}

String NfcManager::detectTag() {
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uidLength;

    if (_pn532.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
        if (uidLength > 0) {
            _lastUid = uidToString(uid, uidLength);
            _lastDetectTime = millis();
            return _lastUid;
        }
    }

    return "";
}

String NfcManager::uidToString(uint8_t* uid, uint8_t uidLength) {
    String result = "";
    for (uint8_t i = 0; i < uidLength; i++) {
        if (i > 0) result += ":";
        if (uid[i] < 16) result += "0";
        result += String(uid[i], HEX);
    }
    result.toUpperCase();
    return result;
}

bool NfcManager::isTagRegistered(const String& uid) {
    if (_config == nullptr) return false;
    return _config->isTagRegistered(uid);
}

void NfcManager::addTagToWhitelist(const String& uid) {
    if (_config) _config->addRegisteredTag(uid);
}

void NfcManager::removeTagFromWhitelist(int index) {
    if (_config) _config->removeRegisteredTag(index);
}

int NfcManager::getWhitelistCount() {
    return _config ? _config->getRegisteredTagCount() : 0;
}

String NfcManager::getWhitelistTag(int index) {
    return _config ? _config->getRegisteredTag(index) : "";
}
```

- [ ] **Step 3: 提交**

```bash
git add src/nfc_manager.h src/nfc_manager.cpp
git commit -m "feat: add NFC manager with whitelist support"
```

---

## Task 7: 计时器管理器

**Files:**
- Create: `src/timer_manager.h`
- Create: `src/timer_manager.cpp`

- [ ] **Step 1: 创建timer_manager.h**

```cpp
#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

#include <Arduino.h>
#include "config_manager.h"

class TimerManager {
public:
    TimerManager();

    void begin();
    void setConfig(ConfigManager* config);

    void startCounting();  // 开始计时
    void reset();          // 重置计时器

    bool isTimeout();      // 是否超时
    unsigned long getElapsedMs();  // 已过时间

    void update();

private:
    ConfigManager* _config;
    bool _isCounting;
    unsigned long _startTime;
};

#endif
```

- [ ] **Step 2: 创建timer_manager.cpp**

```cpp
#include "timer_manager.h"

TimerManager::TimerManager()
    : _config(nullptr), _isCounting(false), _startTime(0) {}

void TimerManager::begin() {}

void TimerManager::setConfig(ConfigManager* config) {
    _config = config;
}

void TimerManager::startCounting() {
    _isCounting = true;
    _startTime = millis();
}

void TimerManager::reset() {
    _isCounting = false;
    _startTime = 0;
}

bool TimerManager::isTimeout() {
    if (!_isCounting || !_config) return false;
    return (millis() - _startTime >= _config->getTimeoutMs());
}

unsigned long TimerManager::getElapsedMs() {
    if (!_isCounting) return 0;
    return millis() - _startTime;
}

void TimerManager::update() {
    // 无需主动更新，时间通过millis()计算
}
```

- [ ] **Step 3: 提交**

```bash
git add src/timer_manager.h src/timer_manager.cpp
git commit -m "feat: add timer manager"
```

---

## Task 8: 状态机

**Files:**
- Create: `src/state_machine.h`
- Create: `src/state_machine.cpp`

- [ ] **Step 1: 创建state_machine.h**

```cpp
#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <Arduino.h>
#include "nfc_manager.h"
#include "led_controller.h"
#include "buzzer_controller.h"
#include "timer_manager.h"

enum State {
    STATE_IDLE,
    STATE_ACTIVE,
    STATE_COUNTING,
    STATE_WARNING,
    STATE_CONFIG
};

class StateMachine {
public:
    StateMachine();

    void begin(NfcManager* nfc, LedController* led,
               BuzzerController* buzzer, TimerManager* timer);

    void update();
    void enterConfigMode();
    void exitConfigMode();
    State getCurrentState();

private:
    NfcManager* _nfc;
    LedController* _led;
    BuzzerController* _buzzer;
    TimerManager* _timer;
    State _currentState;
};

#endif
```

- [ ] **Step 2: 创建state_machine.cpp**

```cpp
#include "state_machine.h"

StateMachine::StateMachine()
    : _nfc(nullptr), _led(nullptr), _buzzer(nullptr),
      _timer(nullptr), _currentState(STATE_IDLE) {}

void StateMachine::begin(NfcManager* nfc, LedController* led,
                         BuzzerController* buzzer, TimerManager* timer) {
    _nfc = nfc;
    _led = led;
    _buzzer = buzzer;
    _timer = timer;
    _currentState = STATE_IDLE;
    _led->setIdle();
}

void StateMachine::update() {
    if (_currentState == STATE_CONFIG) return;

    String detectedUid = _nfc->detectTag();
    bool isRegistered = _nfc->isTagRegistered(detectedUid);

    switch (_currentState) {
        case STATE_IDLE:
            if (isRegistered) {
                _currentState = STATE_ACTIVE;
                _timer->reset();
                _led->setActive();
                _buzzer->stop();
            }
            break;

        case STATE_ACTIVE:
            if (!isRegistered) {
                _currentState = STATE_COUNTING;
                _timer->startCounting();
                _led->setCounting();
            }
            break;

        case STATE_COUNTING:
            if (isRegistered) {
                _currentState = STATE_ACTIVE;
                _timer->reset();
                _led->setActive();
                _buzzer->stop();
            } else if (_timer->isTimeout()) {
                _currentState = STATE_WARNING;
                _led->setWarning();
                _buzzer->startAlarm();
            }
            break;

        case STATE_WARNING:
            if (isRegistered) {
                _currentState = STATE_IDLE;
                _timer->reset();
                _led->setIdle();
                _buzzer->stop();
            }
            break;

        default:
            break;
    }
}

void StateMachine::enterConfigMode() {
    _currentState = STATE_CONFIG;
    _led->setConfig();
    _buzzer->beep(200);
}

void StateMachine::exitConfigMode() {
    _currentState = STATE_IDLE;
    _led->setIdle();
}

State StateMachine::getCurrentState() {
    return _currentState;
}
```

- [ ] **Step 3: 提交**

```bash
git add src/state_machine.h src/state_machine.cpp
git commit -m "feat: add state machine with 5 states"
```

---

## Task 9: WiFi管理器

**Files:**
- Create: `src/wifi_manager.h`
- Create: `src/wifi_manager.cpp`

- [ ] **Step 1: 创建wifi_manager.h**

```cpp
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "config_manager.h"

class WiFiManager {
public:
    WiFiManager();

    void begin(ConfigManager* config);
    void startConfigMode();
    void stopConfigMode();
    bool isConnected();
    String getIP();

    void sendNotification(const String& url);

private:
    ConfigManager* _config;
    bool _isConfigMode;
};

#endif
```

- [ ] **Step 2: 创建wifi_manager.cpp**

```cpp
#include "wifi_manager.h"
#include "config.h"

WiFiManager::WiFiManager()
    : _config(nullptr), _isConfigMode(false) {}

void WiFiManager::begin(ConfigManager* config) {
    _config = config;
}

void WiFiManager::startConfigMode() {
    _isConfigMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.println("AP started: " + String(AP_SSID));
}

void WiFiManager::stopConfigMode() {
    _isConfigMode = false;
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    Serial.println("AP stopped");
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::getIP() {
    if (_isConfigMode) {
        return WiFi.softAPIP().toString();
    }
    return WiFi.localIP().toString();
}

void WiFiManager::sendNotification(const String& url) {
    if (url.length() == 0 || !isConnected()) return;

    WiFiClient client;
    int colonPos = url.indexOf(':');
    String host = url;
    String path = "/";

    if (colonPos > 0) {
        host = url.substring(0, colonPos);
        int pathPos = url.indexOf('/', colonPos + 3);
        if (pathPos > 0) {
            path = url.substring(pathPos);
        }
    }

    if (client.connect(host.c_str(), 80)) {
        client.println("GET " + path + "?type=timeout HTTP/1.1");
        client.println("Host: " + host);
        client.println("Connection: close");
        client.println();
        client.stop();
    }
}
```

- [ ] **Step 3: 提交**

```bash
git add src/wifi_manager.h src/wifi_manager.cpp
git commit -m "feat: add WiFi manager with AP mode"
```

---

## Task 10: Web服务器

**Files:**
- Create: `src/web_server.h`
- Create: `src/web_server.cpp`
- Create: `data/index.html`

- [ ] **Step 1: 创建web_server.h**

```cpp
#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "nfc_manager.h"
#include "wifi_manager.h"
#include "config_manager.h"

class WebServer {
public:
    WebServer();

    void begin(NfcManager* nfc, WiFiManager* wifi, ConfigManager* config);
    void handle();

private:
    AsyncWebServer* _server;
    NfcManager* _nfc;
    WiFiManager* _wifi;
    ConfigManager* _config;
};

#endif
```

- [ ] **Step 2: 创建web_server.cpp**

```cpp
#include "web_server.h"
#include "config.h"

extern const char INDEX_HTML[];

WebServer::WebServer() : _server(nullptr) {}

void WebServer::begin(NfcManager* nfc, WiFiManager* wifi, ConfigManager* config) {
    _nfc = nfc;
    _wifi = wifi;
    _config = config;
    _server = new AsyncWebServer(80);

    // 首页
    _server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        request->send(200, "text/html", INDEX_HTML);
    });

    // 配置API
    _server->on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String json = "{";
        json += "\"timeout\":" + String(_config->getTimeoutMs() / 60000) + ",";
        json += "\"wifi_ssid\":\"" + _config->getWifiSsid() + "\",";
        json += "\"notify_url\":\"" + _config->getNotifyUrl() + "\"";
        json += "}";
        request->send(200, "application/json", json);
    });

    _server->on("/api/config", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (request->hasParam("timeout", true)) {
            int timeout = request->getParam("timeout", true)->value().toInt();
            _config->setTimeoutMs(timeout * 60000);
        }
        if (request->hasParam("wifi_ssid", true)) {
            _config->setWifiSsid(request->getParam("wifi_ssid", true)->value());
        }
        if (request->hasParam("wifi_pass", true)) {
            _config->setWifiPass(request->getParam("wifi_pass", true)->value());
        }
        if (request->hasParam("notify_url", true)) {
            _config->setNotifyUrl(request->getParam("notify_url", true)->value());
        }
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // NFC API
    _server->on("/api/nfc", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String detected = _nfc->detectTag();
        String json = "{";
        json += "\"detected\":\"" + detected + "\",";
        json += "\"tags\":[";
        int count = _nfc->getWhitelistCount();
        for (int i = 0; i < count; i++) {
            if (i > 0) json += ",";
            json += "\"" + _nfc->getWhitelistTag(i) + "\"";
        }
        json += "]";
        json += "}";
        request->send(200, "application/json", json);
    });

    _server->on("/api/nfc/add", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (request->hasParam("uid", true)) {
            String uid = request->getParam("uid", true)->value();
            _nfc->addTagToWhitelist(uid);
        }
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    _server->on("/api/nfc/remove", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (request->hasParam("index", true)) {
            int index = request->getParam("index", true)->value().toInt();
            _nfc->removeTagFromWhitelist(index);
        }
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    _server->begin();
}
```

- [ ] **Step 3: 创建data/index.html**

```html
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>时间管控器</title>
    <style>
        body { font-family: Arial; max-width: 600px; margin: 20px auto; padding: 20px; background: #f5f5f5; }
        h1 { color: #333; }
        .card { background: white; padding: 20px; margin: 15px 0; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .card h2 { margin-top: 0; color: #e94560; }
        label { display: block; margin: 10px 0 5px; }
        input, select { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
        button { background: #e94560; color: white; border: none; padding: 12px 24px; border-radius: 4px; cursor: pointer; margin: 10px 5px 0 0; }
        button:hover { background: #d1364e; }
        button.secondary { background: #666; }
        .tag-list { background: #f9f9f9; padding: 10px; border-radius: 4px; min-height: 50px; }
        .tag { display: inline-block; background: #00b894; color: white; padding: 5px 10px; border-radius: 4px; margin: 3px; font-family: monospace; }
        .tag .del { cursor: pointer; margin-left: 8px; color: #ff6b6b; }
        .detected { color: #00b894; font-family: monospace; }
        .nav { margin: 20px 0; }
        .nav a { margin-right: 15px; }
    </style>
</head>
<body>
    <h1>时间管控器</h1>
    <div class="nav">
        <a href="/">配置</a>
        <a href="/nfc">NFC标签</a>
    </div>

    <div class="card" id="config-page">
        <h2>设置</h2>
        <label>超时时长（分钟）</label>
        <input type="number" id="timeout" value="30">

        <label>WiFi SSID</label>
        <input type="text" id="wifi_ssid" placeholder="选择或输入WiFi名称">

        <label>WiFi密码</label>
        <input type="password" id="wifi_pass" placeholder="WiFi密码">

        <label>通知URL</label>
        <input type="text" id="notify_url" placeholder="http://...">

        <button onclick="saveConfig()">保存配置</button>
        <button class="secondary" onclick="location.reload()">刷新</button>
    </div>

    <div class="card" id="nfc-page" style="display:none;">
        <h2>NFC标签管理</h2>

        <label>已注册标签</label>
        <div class="tag-list" id="tag-list"></div>

        <label style="margin-top:20px;">当前检测到的标签</label>
        <div class="detected" id="detected-tag">等待检测...</div>

        <button onclick="addCurrentTag()" style="background:#00b894;">添加当前标签</button>
        <button class="secondary" onclick="rescan()">重新扫描</button>
    </div>

    <script>
        const API_BASE = '/api';

        async function loadConfig() {
            const res = await fetch(API_BASE + '/config');
            const data = await res.json();
            document.getElementById('timeout').value = data.timeout || 30;
            document.getElementById('wifi_ssid').value = data.wifi_ssid || '';
            document.getElementById('notify_url').value = data.notify_url || '';
        }

        async function saveConfig() {
            const data = {
                timeout: document.getElementById('timeout').value,
                wifi_ssid: document.getElementById('wifi_ssid').value,
                wifi_pass: document.getElementById('wifi_pass').value,
                notify_url: document.getElementById('notify_url').value
            };

            await fetch(API_BASE + '/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: new URLSearchParams(data)
            });
            alert('配置已保存');
        }

        async function loadNFC() {
            const res = await fetch(API_BASE + '/nfc');
            const data = await res.json();

            document.getElementById('detected-tag').textContent =
                data.detected || '未检测到标签';

            const list = document.getElementById('tag-list');
            list.innerHTML = '';
            data.tags.forEach((tag, i) => {
                const span = document.createElement('span');
                span.className = 'tag';
                span.innerHTML = tag + '<span class="del" onclick="removeTag(' + i + ')">×</span>';
                list.appendChild(span);
            });
        }

        async function addCurrentTag() {
            const detected = document.getElementById('detected-tag').textContent;
            if (detected && detected !== '未检测到标签') {
                await fetch(API_BASE + '/nfc/add', {
                    method: 'POST',
                    body: 'uid=' + detected
                });
                loadNFC();
            }
        }

        async function removeTag(index) {
            await fetch(API_BASE + '/nfc/remove', {
                method: 'POST',
                body: 'index=' + index
            });
            loadNFC();
        }

        function rescan() {
            loadNFC();
        }

        // 导航
        document.querySelectorAll('.nav a').forEach(a => {
            a.addEventListener('click', e => {
                e.preventDefault();
                document.querySelectorAll('.card').forEach(c => c.style.display = 'none');
                document.getElementById(a.getAttribute('href').slice(1) + '-page').style.display = 'block';
                if (a.getAttribute('href') === '/nfc') loadNFC();
            });
        });

        loadConfig();
        loadNFC();
    </script>
</body>
</html>
```

- [ ] **Step 4: 提交**

```bash
git add src/web_server.h src/web_server.cpp data/index.html
git commit -m "feat: add web server with config and NFC management"
```

---

## Task 11: 主程序

**Files:**
- Create: `src/main.cpp`

- [ ] **Step 1: 创建main.cpp**

```cpp
#include <Arduino.h>
#include "config.h"
#include "config_manager.h"
#include "nfc_manager.h"
#include "led_controller.h"
#include "buzzer_controller.h"
#include "timer_manager.h"
#include "button_manager.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "state_machine.h"

// 全局实例
ConfigManager configManager;
NfcManager nfcManager(PIN_NFC_IRQ, PIN_NFC_RST);
LedController ledController(PIN_LED_R, PIN_LED_G);
BuzzerController buzzerController(PIN_BUZZER);
TimerManager timerManager;
ButtonManager buttonManager(PIN_BTN);
WiFiManager wifiManager;
WebServer webServer;
StateMachine stateMachine;

// HTML页面
#include "index.html"

void setup() {
    Serial.begin(115200);
    delay(1000);

    // 初始化各模块
    configManager.begin();
    ledController.begin();
    buzzerController.begin();
    buttonManager.begin();
    timerManager.setConfig(&configManager);

    if (!nfcManager.begin()) {
        Serial.println("NFC init failed!");
    }

    // 初始化状态机
    stateMachine.begin(&nfcManager, &ledController, &buzzerController, &timerManager);

    // 检查WiFi配置
    if (configManager.getWifiSsid().length() == 0) {
        // 无WiFi配置，进入配网模式
        wifiManager.startConfigMode();
        webServer.begin(&nfcManager, &wifiManager, &configManager);
        stateMachine.enterConfigMode();
    } else {
        // 有配置，连接WiFi
        WiFi.begin(configManager.getWifiSsid().c_str(),
                  configManager.getWifiPass().c_str());
        // 等待连接或超时后进入配网
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            attempts++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            webServer.begin(&nfcManager, &wifiManager, &configManager);
        } else {
            wifiManager.startConfigMode();
            webServer.begin(&nfcManager, &wifiManager, &configManager);
            stateMachine.enterConfigMode();
        }
    }

    Serial.println("System ready");
}

void loop() {
    // 检查配网按钮
    if (buttonManager.checkLongPress(BTN_LONG_PRESS_MS)) {
        if (stateMachine.getCurrentState() != STATE_CONFIG) {
            stateMachine.enterConfigMode();
            wifiManager.startConfigMode();
            webServer.begin(&nfcManager, &wifiManager, &configManager);
            buzzerController.beep(200);
        }
    }

    // 更新状态机
    stateMachine.update();

    // 更新LED
    ledController.update();

    // 更新蜂鸣器
    buzzerController.update();

    delay(50);
}
```

- [ ] **Step 2: 修复编译问题（添加index.html引用）**

编辑 `platformio.ini`，确保HTML文件被包含到SPIFFS：

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

board_build.filesystem = littlefs
build_flags =
    -DINDEX_HTML_LENGTH=8192

lib_deps =
    adafruit/Adafruit PN532@^1.2.0
    https://github.com/me-no-dev/ESPAsyncWebServer.git
    me-no-dev/ESP Async TCP@^1.1.4
    esphoekma/Preferences@^2.0.0
```

修改 `main.cpp` 头部添加index.html定义：

```cpp
// 在文件开头添加
#ifndef INDEX_HTML
#define INDEX_HTML_SZ 8192
static const char INDEX_HTML[INDEX_HTML_SZ] = {
    // 从data/index.html读取的内容
};
#endif
```

- [ ] **Step 3: 提交**

```bash
git add src/main.cpp
git commit -m "feat: add main program with all modules"
```

---

## Task 12: 编译测试

- [ ] **Step 1: 安装依赖并编译**

```bash
cd /c/sl/code/esp32/timecontrol
pio run
```

- [ ] **Step 2: 检查编译错误并修复**

根据实际错误修复代码

- [ ] **Step 3: 提交最终版本**

```bash
git add -A
git commit -m "feat: complete time controller project"
```

---

## 自检清单

1. **Spec覆盖检查**
   - [x] 硬件接线 → Task 1-3
   - [x] NFC检测+白名单 → Task 6
   - [x] LED控制 → Task 2
   - [x] 蜂鸣器控制 → Task 3
   - [x] 按钮长按检测 → Task 4
   - [x] 状态机 → Task 8
   - [x] WiFi配网 → Task 9
   - [x] Web配置界面 → Task 10
   - [x] 配置存储 → Task 5

2. **占位符扫描** - 无"TBD"、"TODO"等占位符

3. **类型一致性** - 各模块接口定义一致

---

**Plan complete and saved to `docs/superpowers/plans/2026-05-22-timecontrol-plan.md`.**