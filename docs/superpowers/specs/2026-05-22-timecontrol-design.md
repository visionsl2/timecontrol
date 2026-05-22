# 时间管控器 - 设计规范 V1.1

## 概述

基于ESP32的时间管控设备，通过NFC检测控制Switch游戏机使用时间。检测到已注册的NFC标签在盒内时正常，离开超过设定时间后触发声光报警。

---

## 硬件设计

### BOM清单

| 组件 | 型号 | 数量 | 备注 |
|-----|------|------|------|
| 主控板 | ESP32 DEVKIT V1 | 1 | 含WiFi功能 |
| NFC模块 | PN532 | 1 | I2C接口 |
| NFC标签 | NTAG215 | 1 | 贴在Switch背面 |
| 双色LED | 共阳极红绿 | 1 | 串联220Ω电阻 |
| 有源蜂鸣器 | 5V 有源 | 1 | 通电即响 |
| 电阻 | 220Ω | 2 | LED限流 |
| 微动按钮 | 6x6x5mm | 1 | 配网按钮 |

### 接线方案

```
ESP32  →  PN532 (I2C)
─────────────────────────────────
3.3V   →  VCC
GND    →  GND
GPIO21 →  SDA
GPIO22 →  SCL
GPIO4  →  IRQ
GPIO5  →  RST

ESP32  →  指示器件
─────────────────────────────────
GPIO12 →  LED_R (串联220Ω电阻)
GPIO13 →  LED_G (串联220Ω电阻)
GPIO14 →  有源蜂鸣器 (Signal)
GND    →  蜂鸣器 GND

ESP32  →  按钮
─────────────────────────────────
GPIO0  →  微动按钮 (另一端接地)
```

---

## 软件设计

### 状态机

| 状态 | LED表现 | 说明 |
|-----|--------|------|
| IDLE | 红绿交替闪烁 | 初始状态/等待 |
| ACTIVE | 绿灯常亮 | Switch在盒内 |
| COUNTING | 绿灯闪烁（1秒间隔） | Switch不在盒内，开始计时 |
| WARNING | 红灯闪烁 | 超时报警 |
| CONFIG | 红绿交替闪烁 | 配网模式 |

### 状态转换

```
IDLE ──检测到已注册NFC──→ ACTIVE
ACTIVE ──检测不到NFC──→ COUNTING
COUNTING ──超时──→ WARNING
COUNTING ──重新检测到NFC──→ ACTIVE
WARNING ──重新检测到NFC──→ IDLE
任意状态 ──长按按钮3秒──→ CONFIG
CONFIG ──保存配置──→ IDLE
```

### 核心功能模块

| 模块 | 功能 | 文件 |
|-----|------|------|
| NfcManager | NFC读写、标签检测、UID白名单验证 | nfc_manager.h/cpp |
| LedController | 双色LED控制、闪烁模式 | led_controller.h/cpp |
| BuzzerController | 蜂鸣器报警控制 | buzzer_controller.h/cpp |
| TimerManager | 超时计时、状态管理 | timer_manager.h/cpp |
| ButtonManager | 配网按钮检测（长按3秒触发） | button_manager.h/cpp |
| WiFiManager | WiFi连接、配网 | wifi_manager.h/cpp |
| WebServer | 配置页面、状态显示 | web_server.h/cpp |
| ConfigManager | 配置存储(Preferences/NVS) | config_manager.h/cpp |

---

## WiFi配置功能

### 配网触发

| 方式 | 条件 |
|-----|------|
| 自动 | 首次上电无WiFi配置时自动进入AP模式 |
| 手动 | 长按配网按钮超过3秒 |

### Web页面

**配置页面 /config**
- 超时时长设置
- WiFi SSID/密码
- 通知URL

**NFC管理页面 /nfc**
- 已注册标签列表（支持删除）
- 当前检测到的标签UID
- 添加当前标签到白名单

### 远程通知

超时触发时发送HTTP GET请求：
```
GET /notify?type=timeout&device=switch-control-01 HTTP/1.1
Host: your-server.com
```

---

## 开发计划

### 阶段1：基础功能
- [ ] 硬件接线验证
- [ ] PN532 NFC模块驱动
- [ ] LED控制
- [ ] 蜂鸣器控制
- [ ] 配网按钮驱动
- [ ] 状态机实现
- [ ] 计时逻辑
- [ ] NFC UID白名单

### 阶段2：WiFi功能
- [ ] WiFiManager配网
- [ ] WebServer配置界面
- [ ] NFC管理页面
- [ ] 配置存储
- [ ] 远程通知

### 阶段3：优化完善
- [ ] LED闪烁节奏
- [ ] 蜂鸣器报警模式
- [ ] 外壳设计（可选）

---

## 技术选型

| 项目 | 选择 | 理由 |
|-----|------|------|
| 开发框架 | PlatformIO + Arduino | 生态完善 |
| NFC库 | Adafruit_PN532 | 文档完善 |
| Web服务器 | ESP Async WebServer | 异步高效 |
| 配置存储 | Preferences库 | NVS封装 |
| 配网方案 | 自实现Captive Portal | 轻量可控 |

---

## 项目结构

```
timecontrol/
├── src/
│   ├── main.cpp
│   ├── config.h
│   ├── nfc_manager.h/cpp
│   ├── led_controller.h/cpp
│   ├── buzzer_controller.h/cpp
│   ├── timer_manager.h/cpp
│   ├── button_manager.h/cpp
│   ├── wifi_manager.h/cpp
│   ├── web_server.h/cpp
│   └── config_manager.h/cpp
├── data/
│   └── index.html
├── platformio.ini
└── README.md
```