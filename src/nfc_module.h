/**
 * NFC独立模块 - PN532中断驱动 + 防抖
 *
 * 功能：检测NFC标签，处理GPIO4中断，防抖机制
 * 防抖：中断后延迟50ms读取，再延迟2000ms防止频繁中断
 */

#ifndef NFC_MODULE_H
#define NFC_MODULE_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PN532.h>

// ==================== 常量定义 ====================
#define NFC_DEBOUNCE1_MS   50      // 首次延迟，等待NFC稳定
#define NFC_DEBOUNCE2_MS   2000    // 二次延迟，防止频繁中断
#define NFC_READ_TIMEOUT   500     // 读取超时500ms
#define NFC_IRQ_PIN        4       // GPIO4中断引脚
#define NFC_SDA_PIN        1       // I2C SDA
#define NFC_SCL_PIN        2       // I2C SCL

// ==================== 回调类型 ====================
typedef void (*NfcEventCallback)(bool present, String uid);

// ==================== NFC模块类 ====================
class NfcModule {
private:
    Adafruit_PN532* _nfc;
    volatile bool _interruptFlag;       // 中断触发标志
    bool _nfcPresent;                    // 当前NFC状态
    String _lastUid;                    // 上次读取的UID
    unsigned long _lastEventTime;        // 上次事件时间
    NfcEventCallback _callback;          // 回调函数
    bool _initialized;                   // 是否已初始化
    bool _processing;                    // 是否正在处理中断

    // 中断服务程序
    static void nfcISR();

public:
    NfcModule();

    // 初始化NFC模块
    bool begin();

    // 读取NFC标签
    bool readTag(String& uid);

    // 设置事件回调
    void onEvent(NfcEventCallback callback);

    // 获取当前NFC状态
    bool isPresent();

    // 获取上次读取的UID
    String getLastUid();

    // 处理中断（在loop中调用）
    void processInterrupt();

    // 主动轮询检测（备用）
    bool poll();

    // 检查是否正在处理
    bool isProcessing() { return _processing; }
};

#endif // NFC_MODULE_H