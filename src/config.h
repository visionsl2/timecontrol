#ifndef CONFIG_H
#define CONFIG_H

// 引脚定义
#define PIN_NFC_IRQ   4
#define PIN_NFC_RST   5
#define PIN_LED_R     12
#define PIN_LED_G     13
#define PIN_BUZZER    14
#define PIN_BTN       0

// 超时默认30分钟
#define DEFAULT_TIMEOUT_MS (30 * 60 * 1000)

// LED闪烁间隔
#define LED_BLINK_INTERVAL 1000

// 按钮长按时间3秒
#define BTN_LONG_PRESS_MS 3000

// WiFi AP配置
#define AP_SSID "SwitchTimeControl"
#define AP_PASS "12345678"

// 最大注册标签数
#define MAX_REGISTERED_TAGS 10

// NVS键名
#define NVS_NAMESPACE "timecontrol"
#define NVS_KEY_WIFI_SSID "wifi_ssid"
#define NVS_KEY_WIFI_PASS "wifi_pass"
#define NVS_KEY_TIMEOUT "timeout_ms"
#define NVS_KEY_NOTIFY_URL "notify_url"
#define NVS_KEY_TAG_COUNT "tag_count"
#define NVS_KEY_TAG_PREFIX "tag_"

#endif