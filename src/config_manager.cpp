#include "config_manager.h"
#include "config.h"

ConfigManager::ConfigManager() {}

bool ConfigManager::begin() {
    return _prefs.begin(NVS_NAMESPACE, false);
}

void ConfigManager::end() {
    _prefs.end();
}

bool ConfigManager::setWiFiCredentials(const String& ssid, const String& password) {
    if (ssid.isEmpty()) {
        return false;
    }

    bool success = _prefs.putString(NVS_KEY_WIFI_SSID, ssid.c_str());
    if (!success) {
        return false;
    }

    if (!password.isEmpty()) {
        success = _prefs.putString(NVS_KEY_WIFI_PASS, password.c_str());
    }
    return success;
}

bool ConfigManager::getWiFiSsid(String& outSsid) const {
    outSsid = _prefs.getString(NVS_KEY_WIFI_SSID, "");
    return !outSsid.isEmpty();
}

bool ConfigManager::getWiFiPassword(String& outPassword) const {
    outPassword = _prefs.getString(NVS_KEY_WIFI_PASS, "");
    return true;
}

bool ConfigManager::isWiFiConfigured() const {
    String ssid;
    return getWiFiSsid(ssid);
}

bool ConfigManager::setTimeout(uint32_t timeoutMs) {
    return _prefs.putUInt(NVS_KEY_TIMEOUT, timeoutMs);
}

bool ConfigManager::getTimeout(uint32_t& outTimeoutMs) const {
    outTimeoutMs = _prefs.getUInt(NVS_KEY_TIMEOUT, DEFAULT_TIMEOUT_MS);
    return true;
}

bool ConfigManager::setNotificationUrl(const String& url) {
    return _prefs.putString(NVS_KEY_NOTIFY_URL, url.c_str());
}

bool ConfigManager::getNotificationUrl(String& outUrl) const {
    outUrl = _prefs.getString(NVS_KEY_NOTIFY_URL, "");
    return true;
}

String ConfigManager::_getTagKey(uint8_t index) const {
    return String(NVS_KEY_TAG_PREFIX) + String(index);
}

String ConfigManager::_getTagKeyByUid(const String& uid) const {
    // 存储时使用UID本身作为键值，方便查找
    return String(NVS_KEY_TAG_PREFIX) + uid;
}

bool ConfigManager::addRegisteredTag(const String& tagUid) {
    if (tagUid.isEmpty()) {
        return false;
    }

    // 检查是否已存在
    if (isTagRegistered(tagUid)) {
        return true;  // 已经注册，无需重复添加
    }

    // 获取当前标签数量
    uint8_t currentCount = getRegisteredTagCount();

    // 检查是否达到上限
    if (currentCount >= MAX_REGISTERED_TAGS) {
        return false;
    }

    // 在指定位置存储新标签
    String key = _getTagKey(currentCount);
    if (!_prefs.putString(key.c_str(), tagUid.c_str())) {
        return false;
    }

    // 增加标签计数
    return _prefs.putUChar(NVS_KEY_TAG_COUNT, currentCount + 1);
}

bool ConfigManager::removeRegisteredTag(const String& tagUid) {
    if (tagUid.isEmpty()) {
        return false;
    }

    std::vector<String> tags;
    if (!getRegisteredTags(tags)) {
        return false;
    }

    // 找到要删除的标签索引
    int removeIndex = -1;
    for (int i = 0; i < tags.size(); i++) {
        if (tags[i] == tagUid) {
            removeIndex = i;
            break;
        }
    }

    if (removeIndex < 0) {
        return false;  // 标签不存在
    }

    // 将最后一个标签移动到删除位置
    uint8_t count = getRegisteredTagCount();
    if (removeIndex < count - 1) {
        String lastTag = tags[count - 1];
        String key = _getTagKey(removeIndex);
        _prefs.putString(key.c_str(), lastTag.c_str());
    }

    // 清除最后一个位置
    String lastKey = _getTagKey(count - 1);
    _prefs.remove(lastKey.c_str());

    // 减少计数
    return _prefs.putUChar(NVS_KEY_TAG_COUNT, count - 1);
}

bool ConfigManager::isTagRegistered(const String& tagUid) const {
    std::vector<String> tags;
    if (!getRegisteredTags(tags)) {
        return false;
    }

    for (const auto& tag : tags) {
        if (tag == tagUid) {
            return true;
        }
    }
    return false;
}

bool ConfigManager::getRegisteredTags(std::vector<String>& outTags) const {
    outTags.clear();

    uint8_t count = getRegisteredTagCount();
    for (uint8_t i = 0; i < count; i++) {
        String key = _getTagKey(i);
        String tag = _prefs.getString(key.c_str(), "");
        if (!tag.isEmpty()) {
            outTags.push_back(tag);
        }
    }
    return true;
}

uint8_t ConfigManager::getRegisteredTagCount() const {
    return _prefs.getUChar(NVS_KEY_TAG_COUNT, 0);
}

bool ConfigManager::clearAllTags() {
    std::vector<String> tags;
    if (getRegisteredTags(tags)) {
        for (const auto& tag : tags) {
            String key = _getTagKeyByUid(tag);
            _prefs.remove(key.c_str());
        }
    }
    return _prefs.putUChar(NVS_KEY_TAG_COUNT, 0);
}