#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <vector>
#include <String>

class ConfigManager {
public:
    ConfigManager();

    bool begin();
    void end();

    // WiFi配置
    bool setWiFiCredentials(const String& ssid, const String& password);
    bool getWiFiSsid(String& outSsid) const;
    bool getWiFiPassword(String& outPassword) const;
    bool isWiFiConfigured() const;

    // 超时配置
    bool setTimeout(uint32_t timeoutMs);
    bool getTimeout(uint32_t& outTimeoutMs) const;

    // 通知URL配置
    bool setNotificationUrl(const String& url);
    bool getNotificationUrl(String& outUrl) const;

    // NFC标签管理
    bool addRegisteredTag(const String& tagUid);
    bool removeRegisteredTag(const String& tagUid);
    bool isTagRegistered(const String& tagUid) const;
    bool getRegisteredTags(std::vector<String>& outTags) const;
    uint8_t getRegisteredTagCount() const;
    bool clearAllTags();

private:
    Preferences _prefs;

    String _getTagKey(uint8_t index) const;
    String _getTagKeyByUid(const String& uid) const;
};

#endif