#include "web_server.h"
#include "config_manager.h"
#include "nfc_manager.h"

// Helper function to parse URL-encoded form data
static String getValueFromBody(const String& body, const String& key) {
    int keyStart = body.indexOf(key + "=");
    if (keyStart == -1) return "";

    int valueStart = keyStart + key.length() + 1;
    int valueEnd = body.indexOf("&", valueStart);
    if (valueEnd == -1) valueEnd = body.length();

    return body.substring(valueStart, valueEnd);
}

WebServer::WebServer() : _server(nullptr), _config(nullptr), _nfc(nullptr) {
}

void WebServer::begin(ConfigManager* config, NfcManager* nfc) {
    _config = config;
    _nfc = nfc;

    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed");
        return;
    }

    // Create web server on port 80
    _server = new AsyncWebServer(80);

    setupRoutes();

    _server->begin();
    Serial.println("Web server started");
}

void WebServer::end() {
    if (_server) {
        _server->end();
        delete _server;
        _server = nullptr;
    }
    SPIFFS.end();
}

void WebServer::setupRoutes() {
    // Serve index.html
    _server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (SPIFFS.exists("/index.html")) {
            request->send(SPIFFS, "/index.html", "text/html");
        } else {
            request->send(404, "text/plain", "index.html not found");
        }
    });

    // API: Get config
    _server->on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String json = getConfigJson(_config);
        request->send(200, "application/json", json);
    });

    // API: Set config
    _server->on("/api/config", HTTP_POST, [this](AsyncWebServerRequest *request) {
        String body = "";
        if (request->hasParam("body", true)) {
            body = request->getParam("body", true)->value();
        }

        String ssid, pass, notifyUrl;
        uint32_t timeout = 0;

        if (parseConfigPost(body, ssid, pass, timeout, notifyUrl)) {
            if (!ssid.isEmpty()) {
                _config->setWiFiSsid(ssid);
                _config->setWiFiPassword(pass);
            }
            if (timeout > 0) {
                _config->setTimeoutMs(timeout);
            }
            if (!notifyUrl.isEmpty()) {
                _config->setNotifyUrl(notifyUrl);
            }
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"invalid request\"}");
        }
    });

    // API: Get NFC status
    _server->on("/api/nfc", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String json = getNfcJson(_nfc);
        request->send(200, "application/json", json);
    });

    // API: Add NFC tag
    _server->on("/api/nfc/add", HTTP_POST, [this](AsyncWebServerRequest *request) {
        String body = "";
        if (request->hasParam("body", true)) {
            body = request->getParam("body", true)->value();
        }

        String uid = parseUidFromBody(body);
        if (!uid.isEmpty()) {
            _nfc->addTagToWhitelist(uid);
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"uid required\"}");
        }
    });

    // API: Remove NFC tag
    _server->on("/api/nfc/remove", HTTP_POST, [this](AsyncWebServerRequest *request) {
        String body = "";
        if (request->hasParam("body", true)) {
            body = request->getParam("body", true)->value();
        }

        int index = parseIndexFromBody(body);
        if (index >= 0) {
            _nfc->removeTagFromWhitelist(index);
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"error\":\"index required\"}");
        }
    });

    // Serve static files from SPIFFS
    _server->serveStatic("/static/", SPIFFS, "/static/");
}

String WebServer::getConfigJson(ConfigManager* config) {
    String ssid = config->getWiFiSsid();
    uint32_t timeoutMs = config->getTimeoutMs();
    String notifyUrl = config->getNotifyUrl();

    // Convert timeout from ms to minutes
    uint32_t timeoutMin = timeoutMs / 60000;

    String json = "{";
    json += "\"timeout\":" + String(timeoutMin) + ",";
    json += "\"wifi_ssid\":\"" + ssid + "\",";
    json += "\"notify_url\":\"" + notifyUrl + "\"";
    json += "}";

    return json;
}

bool WebServer::parseConfigPost(const String& body, String& ssid, String& pass, uint32_t& timeout, String& notifyUrl) {
    ssid = getValueFromBody(body, "wifi_ssid");
    pass = getValueFromBody(body, "wifi_pass");
    String timeoutStr = getValueFromBody(body, "timeout");
    notifyUrl = getValueFromBody(body, "notify_url");

    if (!timeoutStr.isEmpty()) {
        timeout = timeoutStr.toInt() * 60000;  // Convert minutes to ms
    }

    return true;  // Allow partial updates
}

String WebServer::getNfcJson(NfcManager* nfc) {
    String detected = nfc->detectTag();
    int count = nfc->getWhitelistCount();

    String json = "{";
    json += "\"detected\":\"" + detected + "\",";
    json += "\"tags\":[";

    for (int i = 0; i < count; i++) {
        if (i > 0) json += ",";
        json += "\"" + nfc->getWhitelistTag(i) + "\"";
    }

    json += "]";
    json += "}";

    return json;
}

String WebServer::parseUidFromBody(const String& body) {
    return getValueFromBody(body, "uid");
}

int WebServer::parseIndexFromBody(const String& body) {
    String indexStr = getValueFromBody(body, "index");
    if (indexStr.isEmpty()) return -1;
    return indexStr.toInt();
}