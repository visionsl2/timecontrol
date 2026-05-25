# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

ESP32-S3 时间管控器，用于管理Nintendo Switch游戏机使用时间。
- **核心功能**：检测NFC标签，判断Switch是否在盒内，超时报警
- **状态机**：IDLE → ACTIVE → COUNTING → WARNING → CONFIG

## 开发环境

### PlatformIO（推荐）
```bash
pio run              # 编译
pio run --target upload  # 上传固件
pio run --monitor    # 串口监视器（115200波特率）
```

### Arduino IDE
- 主文件：`timecontrol.ino`（单文件 sketch）
- 头文件：`html_page.h`（必须与.ino在同一目录）
- 开发板：ESP32S3 Dev Module
- 上传端口：USB口
- 串口监视器：UART口（ESP32-S3有两个USB口）
- **注意**：Arduino IDE编译时源码必须在项目根目录，不能在 `src/` 子目录下

## 硬件接线

| ESP32引脚 | 功能 | 备注 |
|-----------|------|------|
| GPIO1 | NFC SDA | I2C数据 |
| GPIO2 | NFC SCL | I2C时钟 |
| GPIO12 | LED 红色 | 串联220Ω电阻 |
| GPIO13 | LED 绿色 | 串联220Ω电阻 |
| GPIO14 | 蜂鸣器 | 有源5V，低电平触发 |
| GPIO0 | 配置按钮 | 内部上拉，长按3秒触发 |

### PN532 NFC模块
- 小板版本：仅4个引脚（3.3V, GND, SDA, SCL）
- 无IRQ/RST引脚（代码中使用255表示）
- 支持：NTAG215/216、Mifare Classic等

### LED说明
- 共阳极双色LED
- 高电平点亮对应颜色
- STATE_IDLE/CONFIG：红绿交替闪烁
- STATE_ACTIVE：绿灯常亮
- STATE_COUNTING：绿灯闪烁
- STATE_WARNING：红灯闪烁

## 代码架构

```
timecontrol.ino (主程序)
├── 配置区：引脚定义、时间常量、WiFi凭证
├── 状态枚举：State
├── 全局变量：Preferences、NFC、LED、蜂鸣器状态
├── 配置管理：WiFi/NFC标签的读写
├── NFC模块：detectNFC()、uidToHex()
├── LED控制：ledInit()、ledUpdate()
├── 蜂鸣器：buzzerInit()、buzzerBeep()、buzzerUpdate()
├── Web服务器：setupWeb()、API路由
└── 主程序：setup()、loop()、状态机逻辑

html_page.h（HTML页面）
└── 使用R"rawliteral(...)"避免Arduino预处理器解析JS

platformio.ini（构建配置）
└── ESP32-S3、PlatformIO构建参数
```

## 状态机详解

```
┌─────────────────────────────────────────────────────────┐
│  STATE_IDLE                                              │
│  等待已注册的NFC标签                                      │
│  LED: 红绿交替闪烁                                        │
│  ┌─────────────────┐                                     │
│  │ 检测到已注册标签  │ → STATE_ACTIVE                     │
│  └─────────────────┘                                     │
└─────────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────────┐
│  STATE_ACTIVE                                            │
│  Switch在盒内，计时器停止                                 │
│  LED: 绿灯常亮                                            │
│  ┌──────────────────┐                                    │
│  │ 检测不到NFC标签   │ → STATE_COUNTING                  │
│  └──────────────────┘                                    │
└─────────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────────┐
│  STATE_COUNTING                                          │
│  Switch不在盒内，开始计时                                  │
│  LED: 绿灯闪烁                                           │
│  ┌─────────────┐    ┌─────────────────┐                │
│  │ 标签恢复    │    │  超时           │                │
│  │ → ACTIVE    │    │ → STATE_WARNING │                │
│  └─────────────┘    └─────────────────┘                │
└─────────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────────┐
│  STATE_WARNING                                           │
│  超时报警                                                │
│  LED: 红灯闪烁 + 蜂鸣器响                                 │
│  ┌─────────────────┐                                    │
│  │ 检测到已注册标签  │ → STATE_IDLE                     │
│  └─────────────────┘                                    │
└─────────────────────────────────────────────────────────┘

[长按按钮3秒] → STATE_CONFIG（AP配网模式）
```

## 配置存储（NVS）

| 键名 | 类型 | 说明 |
|------|------|------|
| wifi_ssid | String | WiFi名称 |
| wifi_pass | String | WiFi密码 |
| timeout_ms | ULong | 超时时间（毫秒） |
| notify_url | String | 通知URL |
| tag_count | UChar | 已注册标签数量 |
| tag_0~tag_9 | String | 标签UID |

## Web API

| 端点 | 方法 | 说明 |
|------|------|------|
| / | GET | 返回配置页面HTML |
| /api/config | GET | 获取配置（timeout, wifi_ssid, notify_url） |
| /api/config | POST | 保存配置（timeout, wifi_ssid, wifi_pass, notify_url） |
| /api/nfc | GET | 获取NFC状态（detected当前标签, tags已注册列表） |
| /api/nfc/add | POST | 添加标签（uid参数） |
| /api/nfc/remove | POST | 删除标签（index参数） |

## 关键实现细节

1. **HTML分离**：`html_page.h`使用`R"rawliteral(...)"`语法，避免Arduino预处理器将JavaScript的`function`关键字解析为C++代码

2. **NFC读取冷却**：500ms间隔防止频繁读取

3. **按钮检测**：使用`INPUT_PULLUP`模式，按下为LOW，检测持续时间

4. **NVS持久化**：使用Preferences库，开机后配置保持

5. **双USB口**：ESP32-S3的USB口用于烧录，UART口用于串口监视器

## 已知问题

- PN532模块需要可靠供电，否则可能检测不到
- 首次使用需通过AP模式配置WiFi
- 配置模式热点：`SwitchTimeControl`，密码：`12345678`