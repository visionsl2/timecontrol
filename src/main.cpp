#include <Arduino.h>
#include "config.h"
#include "config_manager.h"
#include "nfc_manager.h"
#include "led_controller.h"
#include "buzzer_controller.h"
#include "timer_manager.h"
#include "button_manager.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "state_machine.h"

// Global instances
ConfigManager configManager;
NfcManager nfcManager(PIN_NFC_IRQ, PIN_NFC_RST);
LedController ledController(PIN_LED_R, PIN_LED_G);
BuzzerController buzzerController(PIN_BUZZER);
TimerManager timerManager;
ButtonManager buttonManager(PIN_BTN);
WiFiManager wifiManager;
WebServer webServer;
StateMachine stateMachine;

// Flag to track if we are in config mode
static bool inConfigMode = false;

// Forward declaration
void enterConfigMode();

void setup() {
    Serial.begin(115200);
    delay(100);

    // Initialize ConfigManager first (storage)
    configManager.begin();

    // Initialize hardware modules
    ledController.begin();
    buzzerController.begin();
    buttonManager.begin();

    // Initialize NFC manager
    nfcManager.begin();
    nfcManager.setConfig(&configManager);

    // Initialize timer manager
    timerManager.begin();
    timerManager.setConfig(&configManager);

    // Initialize State Machine with all dependencies
    stateMachine.begin(&nfcManager, &ledController, &buzzerController, &timerManager);

    // Initialize WiFi manager
    wifiManager.begin(&configManager);

    // Check if WiFi is configured
    String ssid = configManager.getWiFiSsid();
    if (ssid.length() > 0) {
        // WiFi is configured, try to connect
        Serial.println("WiFi configured, connecting to: " + ssid);

        // Wait for WiFi connection (with timeout)
        unsigned long startAttempt = millis();
        while (!wifiManager.isConnected() && (millis() - startAttempt < 10000)) {
            delay(100);
        }

        if (wifiManager.isConnected()) {
            Serial.println("WiFi connected: " + wifiManager.getIP());
            // Not in config mode, normal operation
            inConfigMode = false;
        } else {
            // Connection failed, enter config mode
            Serial.println("WiFi connection failed, entering config mode");
            enterConfigMode();
        }
    } else {
        // No WiFi configured, enter config mode
        Serial.println("No WiFi configured, entering config mode");
        enterConfigMode();
    }
}

void enterConfigMode() {
    // Start WiFi in AP mode
    wifiManager.startConfigMode();

    // Start web server
    webServer.begin(&configManager, &nfcManager);

    // Enter config state
    stateMachine.enterConfigMode();
    inConfigMode = true;

    Serial.println("Config mode started");
    Serial.print("AP IP: ");
    Serial.println(wifiManager.getIP());
}

void loop() {
    // Check for long button press to enter config mode (only if not already in config)
    if (!inConfigMode) {
        if (buttonManager.checkLongPress(BTN_LONG_PRESS_MS)) {
            Serial.println("Long press detected, entering config mode");
            enterConfigMode();
        }
    }

    // Update state machine
    stateMachine.update();

    // Update LED controller (handles blinking)
    ledController.update();

    // Update buzzer controller (handles alarm beeping)
    buzzerController.update();

    // Small delay to prevent busy-waiting
    delay(10);
}