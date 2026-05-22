#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

class ConfigManager;
class NfcManager;

class WebServer {
public:
    WebServer();

    void begin(ConfigManager* config, NfcManager* nfc);
    void end();

private:
    AsyncWebServer* _server;
    ConfigManager* _config;
    NfcManager* _nfc;

    // API handlers
    void setupRoutes();

    // Config API
    static String getConfigJson(ConfigManager* config);
    static bool parseConfigPost(const String& body, String& ssid, String& pass, uint32_t& timeout, String& notifyUrl);

    // NFC API
    static String getNfcJson(NfcManager* nfc);
    static String parseUidFromBody(const String& body);
    static int parseIndexFromBody(const String& body);
};

#endif