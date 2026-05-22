#include "wifi_manager.h"
#include "config.h"
#include "config_manager.h"
#include <HTTPClient.h>

WiFiManager::WiFiManager() : _config(nullptr), _configMode(false) {}

void WiFiManager::begin(ConfigManager* config) {
    _config = config;
    _configMode = false;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
}

void WiFiManager::startConfigMode() {
    if (_configMode) {
        return;  // Already in config mode
    }

    _configMode = true;
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);

    // Start AP with SSID and password from config.h
    WiFi.softAP(AP_SSID, AP_PASS);
}

void WiFiManager::stopConfigMode() {
    if (!_configMode) {
        return;  // Not in config mode
    }

    _configMode = false;
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);

    // Try to connect to target WiFi
    if (_config != nullptr) {
        String ssid = _config->getWiFiSsid();
        String password = _config->getWiFiPassword();
        if (ssid.length() > 0) {
            if (password.length() == 0) {
                WiFi.begin(ssid.c_str());
            } else {
                WiFi.begin(ssid.c_str(), password.c_str());
            }
        }
    }
}

bool WiFiManager::isConnected() {
    if (_configMode) {
        return false;  // Not in STA mode
    }
    return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::getIP() {
    if (_configMode) {
        return WiFi.softAPIP().toString();
    }
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return String();
}

void WiFiManager::sendNotification(const String& url) {
    if (url.isEmpty() || !isConnected()) {
        return;
    }

    HTTPClient http;
    http.begin(url.c_str());
    int httpCode = http.GET();
    http.end();

    // Log result for debugging
    if (httpCode > 0) {
        Serial.printf("[WiFiManager] Notification sent: %d\n", httpCode);
    } else {
        Serial.printf("[WiFiManager] Notification failed: %s\n", http.errorToString(httpCode).c_str());
    }
}