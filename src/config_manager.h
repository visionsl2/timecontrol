#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

class ConfigManager {
public:
    ConfigManager();

    void begin();
    void end();

    // WiFi配置
    String getWiFiSsid();
    void setWiFiSsid(const String& ssid);
    String getWiFiPassword();
    void setWiFiPassword(const String& password);
    bool isWiFiConfigured();

    // 超时配置
    uint32_t getTimeoutMs();
    void setTimeoutMs(uint32_t ms);

    // 通知URL配置
    String getNotifyUrl();
    void setNotifyUrl(const String& url);

    // NFC标签管理
    uint8_t getRegisteredTagCount();
    String getRegisteredTag(uint8_t index);
    void addRegisteredTag(const String& uid);
    void removeRegisteredTag(uint8_t index);
    bool isTagRegistered(const String& uid);
    void clearAllTags();

private:
    Preferences _prefs;
};

#endif