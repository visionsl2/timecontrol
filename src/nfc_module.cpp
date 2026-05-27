/**
 * NFC独立模块实现
 */

#include "nfc_module.h"

// 静态实例指针（用于ISR回调）
static NfcModule* _instance = nullptr;

// ==================== 构造函数 ====================
NfcModule::NfcModule() {
    _nfc = nullptr;
    _interruptFlag = false;
    _nfcPresent = true;  // 初始化假设在位
    _lastUid = "";
    _lastEventTime = 0;
    _callback = nullptr;
    _initialized = false;
    _processing = false;
}

// ==================== 中断服务程序 ====================
void NfcModule::nfcISR() {
    if (_instance) {
        _instance->_interruptFlag = true;
    }
}

// ==================== 初始化 ====================
bool NfcModule::begin() {
    // 初始化I2C
    Wire.begin(NFC_SDA_PIN, NFC_SCL_PIN);
    Wire.setClock(400000);

    // 创建PN532实例
    _nfc = new Adafruit_PN532(NFC_IRQ_PIN, 255, &Wire);

    // 初始化PN532
    Serial.println("[NFC] Initializing PN532...");
    _nfc->begin();

    uint32_t versiondata = _nfc->getFirmwareVersion();
    if (!versiondata) {
        Serial.println("[E] PN532 not found!");
        return false;
    }

    Serial.print("[NFC] PN532 v");
    Serial.print((versiondata >> 16) & 0xFF, DEC);
    Serial.print(".");
    Serial.println((versiondata >> 8) & 0xFF, DEC);

    // 配置SAM
    _nfc->SAMConfig();

    // 配置GPIO4中断
    pinMode(NFC_IRQ_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(NFC_IRQ_PIN), nfcISR, CHANGE);

    _initialized = true;
    _instance = this;
    _lastEventTime = millis();

    Serial.println("[NFC] Interrupt enabled on GPIO4");

    return true;
}

// ==================== 读取NFC标签 ====================
bool NfcModule::readTag(String& uid) {
    if (!_nfc || !_initialized) return false;

    uint8_t uidBuf[10], len;
    if (_nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, uidBuf, &len, NFC_READ_TIMEOUT)) {
        // 转换UID为十六进制字符串
        uid = "";
        for (uint8_t i = 0; i < len; i++) {
            if (i > 0) uid += ":";
            if (uidBuf[i] < 16) uid += "0";
            uid += String(uidBuf[i], HEX);
        }
        uid.toUpperCase();
        return true;
    }
    return false;
}

// ==================== 设置回调 ====================
void NfcModule::onEvent(NfcEventCallback callback) {
    _callback = callback;
}

// ==================== 获取当前状态 ====================
bool NfcModule::isPresent() {
    if (!_initialized) return true;
    return _nfcPresent;
}

// ==================== 获取上次UID ====================
String NfcModule::getLastUid() {
    return _lastUid;
}

// ==================== 处理中断 ====================
void NfcModule::processInterrupt() {
    if (!_initialized || !_interruptFlag) return;

    // 如果正在处理上一次中断，跳过
    if (_processing) return;

    // 清除中断标志
    _interruptFlag = false;
    _processing = true;

    // 延迟50ms等待NFC稳定
    delay(NFC_DEBOUNCE1_MS);

    // 读取NFC
    String uid = "";
    bool present = readTag(uid);


    // 检查状态是否变化
    bool changed = false;
    if (present != _nfcPresent) {
        changed = true;
    } else if (present && uid != _lastUid && _lastUid.length() > 0) {
        // UID变化也视为变化
        changed = true;
    }

    // 更新状态
    _nfcPresent = present;
    if (present) {
        _lastUid = uid;
    }
    _lastEventTime = millis();

    // 输出日志
    if (changed) {
        if (present) {
            Serial.println("[NFC] PRESENT: " + uid);
        } else {
            Serial.println("[NFC] ABSENT");
        }
    }
    
    // 延迟2000ms防止频繁中断
    delay(NFC_DEBOUNCE2_MS);

    // 调用回调
    if (_callback && changed) {
        _callback(present, uid);
    }

    _processing = false;
}

// ==================== 主动轮询 ====================
bool NfcModule::poll() {
    if (!_initialized) return false;

    String uid = "";
    bool present = readTag(uid);

    // 只有状态变化时才更新
    if (present != _nfcPresent) {
        _nfcPresent = present;
        if (present) {
            _lastUid = uid;
        }
        _lastEventTime = millis();

        // 输出日志
        if (present) {
            Serial.println("[NFC] PRESENT: " + uid);
        } else {
            Serial.println("[NFC] ABSENT");
        }
    }

    return present;
}