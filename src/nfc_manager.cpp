#include "nfc_manager.h"
#include "config.h"
#include "config_manager.h"

NfcManager::NfcManager(int irqPin, int rstPin)
    : _irqPin(irqPin), _rstPin(rstPin),
      _pn532(irqPin, rstPin),
      _config(nullptr),
      _lastReadTime(0) {}

bool NfcManager::begin() {
    _pn532.begin();

    uint32_t versiondata = _pn532.getFirmwareVersion();
    if (!versiondata) {
        Serial.println("PN532 not found");
        return false;
    }

    Serial.print("Found PN532 with firmware version: ");
    Serial.print((versiondata >> 16) & 0xFF, DEC);
    Serial.print(".");
    Serial.println((versiondata >> 8) & 0xFF, DEC);

    _pn532.SAMConfig();

    Serial.println("NFC Manager initialized");
    return true;
}

String NfcManager::_uidToHex(const uint8_t* uid, uint8_t uidLength) {
    String result = "";
    for (uint8_t i = 0; i < uidLength; i++) {
        if (i > 0) {
            result += ":";
        }
        if (uid[i] < 0x10) {
            result += "0";
        }
        result += String(uid[i], HEX);
    }
    result.toUpperCase();
    return result;
}

String NfcManager::detectTag() {
    unsigned long now = millis();
    if (now - _lastReadTime < READ_COOLDOWN_MS) {
        return "";
    }

    uint8_t uid[10];
    uint8_t uidLength;

    if (_pn532.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
        _lastReadTime = now;
        return _uidToHex(uid, uidLength);
    }

    _lastReadTime = now;
    return "";
}

bool NfcManager::isTagRegistered(const String& uid) {
    if (_config != nullptr) {
        return _config->isTagRegistered(uid);
    }
    return false;
}

void NfcManager::setConfig(ConfigManager* config) {
    _config = config;
}

void NfcManager::addTagToWhitelist(const String& uid) {
    if (_config != nullptr) {
        _config->addRegisteredTag(uid);
    }
}

void NfcManager::removeTagFromWhitelist(int index) {
    if (_config != nullptr) {
        _config->removeRegisteredTag(index);
    }
}

int NfcManager::getWhitelistCount() {
    if (_config != nullptr) {
        return _config->getRegisteredTagCount();
    }
    return 0;
}

String NfcManager::getWhitelistTag(int index) {
    if (_config != nullptr) {
        return _config->getRegisteredTag(index);
    }
    return "";
}