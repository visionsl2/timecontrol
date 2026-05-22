#ifndef NFC_MANAGER_H
#define NFC_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <String>
#include <Adafruit_PN532.h>

class ConfigManager;

class NfcManager {
public:
    NfcManager(int irqPin, int rstPin);

    bool begin();
    String detectTag();
    bool isTagRegistered(const String& uid);
    void setConfig(ConfigManager* config);
    void addTagToWhitelist(const String& uid);
    void removeTagFromWhitelist(int index);
    int getWhitelistCount();
    String getWhitelistTag(int index);

private:
    int _irqPin;
    int _rstPin;
    Adafruit_PN532 _pn532;
    ConfigManager* _config;
    std::vector<String> _whitelistCache;
    unsigned long _lastReadTime;
    static constexpr unsigned long READ_COOLDOWN_MS = 1000;
    String _uidToHex(const uint8_t* uid, uint8_t uidLength);
};

#endif