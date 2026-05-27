/**
 * WiFi配网模块实现
 */

#include "wifi_module.h"
#include "led_module.h"
#include "nfc_module.h"

// ==================== 构造函数 ====================
WiFiManager::WiFiManager() {
    _server = nullptr;
    _configMode = false;
    _connected = false;
    _led = nullptr;
    _nfc = nullptr;
    _lastBlinkTime = 0;
    _blinkState = false;
    _buttonPin = 0;
    _enterConfigRequested = false;
    _saveNfcUidRequested = false;
}

// ==================== 静态成员定义 ====================
WiFiManager* WiFiManager::_instance = nullptr;
unsigned long WiFiManager::_buttonPressStart = 0;

// ==================== 初始化 ====================
bool WiFiManager::begin() {
    Serial.println("[WiFi] Initializing...");

    // 初始化NVS
    _prefs.begin(WIFI_NAMESPACE, false);

    // 检查是否有WiFi配置
    String ssid = _prefs.getString("ssid", "");
    if (ssid.length() == 0) {
        Serial.println("[WiFi] No WiFi config, entering config mode");
        enterConfigMode();
        return false;
    }

    Serial.println("[WiFi] WiFi config found: " + ssid);
    return true;
}

// ==================== 连接WiFi ====================
bool WiFiManager::connect() {
    String ssid = _prefs.getString("ssid", "");
    String pass = _prefs.getString("pass", "");

    if (ssid.length() == 0) {
        return false;
    }

    Serial.println("[WiFi] Connecting to: " + ssid);

    // 尝试连接
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startTime < WIFI_TIMEOUT_MS) {
        delay(100);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[WiFi] Connected! IP=" + WiFi.localIP().toString());
        _connected = true;
        syncTime();
        return true;
    }

    Serial.println("[WiFi] Connection failed");
    return false;
}

// ==================== 是否已连接 ====================
bool WiFiManager::isConnected() {
    return _connected && WiFi.status() == WL_CONNECTED;
}

// ==================== 是否配网模式 ====================
bool WiFiManager::isConfigMode() {
    return _configMode;
}

// ==================== 进入配网模式 ====================
void WiFiManager::enterConfigMode() {
    _configMode = true;
    _connected = false;

    // 关闭之前可能存在的连接
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);

    // 启动AP
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
    Serial.println("[WiFi] Config mode: " + String(WIFI_AP_SSID) + " IP=192.168.4.1");

    // 设置LED闪烁
    if (_led) {
        _lastBlinkTime = millis();
        _blinkState = false;
    }

    // 设置Web服务器
    setupWebServer();
}

// ==================== 设置LED控制器 ====================
void WiFiManager::setLedController(LedController* led) {
    _led = led;
}

// ==================== 按键中断处理 ====================
void WiFiManager::buttonISR() {
    if (_instance == nullptr) return;

    if (digitalRead(_instance->_buttonPin) == LOW) {
        _buttonPressStart = millis();
    } else {
        unsigned long duration = millis() - _buttonPressStart;
        if (duration >= 3000) {
            Serial.println("[Button] Long press detected");
            _instance->_enterConfigRequested = true;
        } else if (duration >= 100 && _instance->_nfc != nullptr && _instance->_nfc->isPresent()) {
            _instance->_saveNfcUidRequested = true;
        }
        _buttonPressStart = 0;
    }
}

// ==================== 设置按键引脚 ====================
void WiFiManager::setButtonPin(int pin) {
    _buttonPin = pin;
    _instance = this;
    pinMode(_buttonPin, INPUT_PULLUP);
    attachInterrupt(_buttonPin, buttonISR, CHANGE);
    Serial.println("[WiFi] Button configured: GPIO" + String(pin));
}

// ==================== 设置NFC模块 ====================
void WiFiManager::setNfcModule(NfcModule* nfc) {
    _nfc = nfc;
}

// ==================== 处理（loop中调用）====================
void WiFiManager::handle() {
    // 处理pending的按键请求（非中断上下文）
    if (_enterConfigRequested) {
        _enterConfigRequested = false;
        enterConfigMode();
        Serial.println("[Button] Entered config mode");
    }

    if (_saveNfcUidRequested) {
        _saveNfcUidRequested = false;
        if (_nfc != nullptr && _nfc->isPresent()) {
            String uid = _nfc->getLastUid();
            if (uid.length() > 0) {
                _prefs.putString("nfc_uid", uid);
                Serial.println("[Button] NFC UID saved: " + uid);
            }
        }
    }

    if (!_configMode) return;

    // 处理Web请求
    if (_server) {
        _server->handleClient();
    }

    // 配网模式LED交替闪烁
    if (_led && millis() - _lastBlinkTime >= 500) {
        _lastBlinkTime = millis();
        _blinkState = !_blinkState;
        _led->setGreen(_blinkState ? false : true);
        _led->setRed(_blinkState ? true : false);
    }
}

// ==================== 获取本机IP ====================
String WiFiManager::getLocalIP() {
    if (_configMode) {
        return "192.168.4.1";
    }
    return WiFi.localIP().toString();
}

// ==================== NTP对时 ====================
void WiFiManager::syncTime() {
    Serial.println("[WiFi] Syncing time from NTP...");
    configTime(8 * 3600, 0, "ntp7.cloud.aliyuncs.com", "time.nist.gov");
    delay(2000);

    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
    Serial.println("[WiFi] Time synced: " + String(buf));
}

// ==================== Web服务器设置 ====================
void WiFiManager::setupWebServer() {
    _server = new WebServer(80);

    // 首页
    _server->on("/", HTTP_GET, [this]() {
        _server->send(200, "text/html", getConfigHtml());
    });

    // 获取配置
    _server->on("/api/config", HTTP_GET, [this]() {
        String json = "{";
        json += "\"ssid\":\"" + _prefs.getString("ssid", "") + "\"";
        json += "}";
        _server->send(200, "application/json", json);
    });

    // 保存配置
    _server->on("/api/config", HTTP_POST, [this]() {
        String ssid = _server->arg("ssid");
        String pass = _server->arg("pass");

        if (ssid.length() > 0 && pass.length() > 0) {
            _prefs.putString("ssid", ssid);
            _prefs.putString("pass", pass);
            Serial.println("[WiFi] Config saved: " + ssid);

            _server->send(200, "application/json", "{\"status\":\"ok\",\"reboot\":true}");
            delay(500);
            ESP.restart();
        } else {
            _server->send(400, "application/json", "{\"error\":\"invalid params\"}");
        }
    });

    // 重启
    _server->on("/api/reboot", HTTP_POST, [this]() {
        _server->send(200, "application/json", "{\"status\":\"ok\"}");
        delay(100);
        ESP.restart();
    });

    // 扫描WiFi
    _server->on("/api/scan", HTTP_GET, [this]() {
        int n = WiFi.scanComplete();
        if (n == -2) {
            WiFi.scanNetworks(true);
            _server->send(200, "application/json", "{\"scanning\":true}");
        } else if (n >= 0) {
            String json = "[";
            for (int i = 0; i < n; i++) {
                if (i > 0) json += ",";
                json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + WiFi.RSSI(i) + "}";
            }
            json += "]";
            WiFi.scanDelete();
            _server->send(200, "application/json", json);
        } else {
            _server->send(200, "application/json", "[]");
        }
    });

    _server->begin();
    Serial.println("[WiFi] Web server started");
}

// ==================== 配网页面HTML ====================
String WiFiManager::getConfigHtml() {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi配网</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 500px; margin: 50px auto; padding: 20px; background: #f5f5f5; }
        h1 { color: #333; text-align: center; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        select, input { width: 100%; padding: 12px; border: 1px solid #ddd; border-radius: 5px; box-sizing: border-box; }
        button { background: #4CAF50; color: white; padding: 12px 24px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }
        button:hover { background: #45a049; }
        button.secondary { background: #666; }
        .status { padding: 15px; background: #e8f5e9; border-radius: 5px; margin-top: 20px; text-align: center; }
        .hidden-ssid { display: none; }
    </style>
</head>
<body>
    <h1>WiFi配网</h1>
    <div class="form-group">
        <label>选择WiFi网络（自动扫描）</label>
        <select id="ssidSelect" onchange="onSsidSelect()">
            <option value="">-- 扫描中... --</option>
        </select>
        <button class="secondary" onclick="scanWiFi()">重新扫描</button>
    </div>
    <div class="form-group hidden-ssid" id="manualGroup">
        <label>或者手动输入WiFi名称（隐藏SSID）</label>
        <input type="text" id="ssidManual" placeholder="输入WiFi名称">
    </div>
    <div class="form-group">
        <label>WiFi密码</label>
        <input type="password" id="pass" placeholder="输入WiFi密码">
    </div>
    <button onclick="saveConfig()">保存并重启</button>
    <div id="status"></div>
    <script>
        let scanTimeout;
        async function scanWiFi() {
            let select = document.getElementById('ssidSelect');
            select.innerHTML = '<option value="">-- 扫描中... --</option>';
            document.getElementById('manualGroup').style.display = 'block';

            let res = await fetch('/api/scan');
            let data = await res.json();

            if (data.scanning) {
                scanTimeout = setTimeout(scanWiFi, 2000);
                return;
            }

            select.innerHTML = '<option value="">-- 请选择 --</option>';
            for (let net of data) {
                let opt = document.createElement('option');
                opt.value = net.ssid;
                opt.textContent = net.ssid + ' (信号:' + net.rssi + ')';
                select.appendChild(opt);
            }
        }
        function onSsidSelect() {
            let ssid = document.getElementById('ssidSelect').value;
            document.getElementById('ssidManual').value = ssid;
        }
        async function saveConfig() {
            let ssid = document.getElementById('ssidSelect').value || document.getElementById('ssidManual').value;
            let pass = document.getElementById('pass').value;
            if (!ssid || !pass) { alert('请填写完整'); return; }
            let formData = new URLSearchParams();
            formData.append('ssid', ssid);
            formData.append('pass', pass);
            let res = await fetch('/api/config', { method: 'POST', body: formData });
            document.getElementById('status').innerText = '配置已保存，设备正在重启...';
        }
        scanWiFi();
    </script>
</body>
</html>
)rawliteral";
}