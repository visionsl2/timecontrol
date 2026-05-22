#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

class ConfigManager;

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
    bool _configMode;
};

#endif