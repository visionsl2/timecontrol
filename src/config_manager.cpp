#include "config_manager.h"
#include "config.h"

ConfigManager::ConfigManager() {}

void ConfigManager::begin() {
    _prefs.begin(NVS_NAMESPACE, false);
}

void ConfigManager::end() {
    _prefs.end();
}

String ConfigManager::getWiFiSsid() {
    return _prefs.getString(NVS_KEY_WIFI_SSID, "");
}

void ConfigManager::setWiFiSsid(const String& ssid) {
    _prefs.putString(NVS_KEY_WIFI_SSID, ssid.c_str());
}

String ConfigManager::getWiFiPassword() {
    return _prefs.getString(NVS_KEY_WIFI_PASS, "");
}

void ConfigManager::setWiFiPassword(const String& password) {
    _prefs.putString(NVS_KEY_WIFI_PASS, password.c_str());
}

bool ConfigManager::isWiFiConfigured() {
    String ssid = getWiFiSsid();
    return ssid.length() > 0;
}

uint32_t ConfigManager::getTimeoutMs() {
    return _prefs.getUInt(NVS_KEY_TIMEOUT, DEFAULT_TIMEOUT_MS);
}

void ConfigManager::setTimeoutMs(uint32_t ms) {
    _prefs.putUInt(NVS_KEY_TIMEOUT, ms);
}

String ConfigManager::getNotifyUrl() {
    return _prefs.getString(NVS_KEY_NOTIFY_URL, "");
}

void ConfigManager::setNotifyUrl(const String& url) {
    _prefs.putString(NVS_KEY_NOTIFY_URL, url.c_str());
}

uint8_t ConfigManager::getRegisteredTagCount() {
    return _prefs.getUChar(NVS_KEY_TAG_COUNT, 0);
}

String ConfigManager::getRegisteredTag(uint8_t index) {
    if (index >= getRegisteredTagCount()) return "";
    String key = String(NVS_KEY_TAG_PREFIX) + index;
    return _prefs.getString(key.c_str(), "");
}

void ConfigManager::addRegisteredTag(const String& uid) {
    if (isTagRegistered(uid)) return;
    uint8_t count = getRegisteredTagCount();
    if (count >= MAX_REGISTERED_TAGS) return;

    String key = String(NVS_KEY_TAG_PREFIX) + count;
    _prefs.putString(key.c_str(), uid.c_str());
    _prefs.putUChar(NVS_KEY_TAG_COUNT, count + 1);
}

void ConfigManager::removeRegisteredTag(uint8_t index) {
    uint8_t count = getRegisteredTagCount();
    if (index >= count) return;

    // 移动后面的标签
    for (uint8_t i = index; i < count - 1; i++) {
        String keyCurr = String(NVS_KEY_TAG_PREFIX) + i;
        String keyNext = String(NVS_KEY_TAG_PREFIX) + (i + 1);
        String tag = _prefs.getString(keyNext.c_str(), "");
        _prefs.putString(keyCurr.c_str(), tag.c_str());
    }

    // 删除最后一个
    String lastKey = String(NVS_KEY_TAG_PREFIX) + (count - 1);
    _prefs.remove(lastKey.c_str());
    _prefs.putUChar(NVS_KEY_TAG_COUNT, count - 1);
}

bool ConfigManager::isTagRegistered(const String& uid) {
    uint8_t count = getRegisteredTagCount();
    for (uint8_t i = 0; i < count; i++) {
        if (getRegisteredTag(i) == uid) return true;
    }
    return false;
}

void ConfigManager::clearAllTags() {
    uint8_t count = getRegisteredTagCount();
    for (uint8_t i = 0; i < count; i++) {
        String key = String(NVS_KEY_TAG_PREFIX) + i;
        _prefs.remove(key.c_str());
    }
    _prefs.putUChar(NVS_KEY_TAG_COUNT, 0);
}