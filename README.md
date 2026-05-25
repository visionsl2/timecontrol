# TimeControl - ESP32-S3 时间管控器

一款基于 ESP32-S3 和 PN532 NFC 模块的时间管控设备，用于管理 Nintendo Switch 游戏机使用时间。

## 功能特性

- **NFC检测**：通过 PN532 模块检测贴在 Switch 背面的 NFC 标签，判断游戏机是否在盒内
- **超时提醒**：Switch 离开盒内超过设定时间后，触发 LED 闪烁和蜂鸣器报警
- **白名单管理**：支持通过 Web 页面注册可信的 NFC 标签
- **WiFi配置**：支持 AP 模式配网，也可配置连接现有 WiFi
- **远程通知**：可配置 Webhook URL，超时时发送通知

## 硬件接线

### PN532 NFC 模块（小板版）
```
ESP32-S3  →  PN532
─────────────────────
3.3V      →  VCC
GND       →  GND
GPIO1     →  SDA
GPIO2     →  SCL
```

### LED 和蜂鸣器
```
ESP32-S3  →  组件
─────────────────────
GPIO12    →  红色LED (串联220Ω电阻)
GPIO13    →  绿色LED (串联220Ω电阻)
GPIO14    →  有源蜂鸣器 (Signal)
GND       →  蜂鸣器 GND
```

### 配置按钮
```
ESP32-S3  →  按钮
─────────────────────
GPIO0     →  微动按钮 (另一端接地)
```

## 状态机

```
┌─────────────┐
│   IDLE      │  初始状态，红绿交替闪烁
└──────┬──────┘
       │ 检测到已注册NFC标签
       ▼
┌─────────────┐
│  ACTIVE     │  Switch在盒内，绿灯常亮
└──────┬──────┘
       │ 检测不到NFC标签（20秒锁定后连续10次失败）
       ▼
┌─────────────┐
│  COUNTING   │  开始计时，绿灯闪烁
└──────┬──────┘
       │ 超过设定时间
       ▼
┌─────────────┐
│  WARNING    │  超时报警，红灯闪烁+蜂鸣器响
└──────┬──────┘
       │ 重新检测到NFC标签
       ▼
┌─────────────┐
│   IDLE      │  重置
└─────────────┘

[长按按钮3秒] → CONFIG（AP配网模式）
```

## 开发环境

### Arduino IDE（推荐）
1. 安装 ESP32 板支持（ESP32S3 Dev Module）
2. 安装库：
   - Adafruit PN532
   - Preferences
   - WebServer
3. 打开 `timecontrol.ino`，确保 `html_page.h` 在同一目录
4. 编译上传

### PlatformIO
```bash
pio run          # 编译
pio run --target upload  # 上传
pio run --target monitor  # 串口监视器
```

## 配网

### 首次使用
1. 上电后无 WiFi 配置，设备自动进入 AP 模式
2. 连接热点 `SwitchTimeControl`，密码 `12345678`
3. 打开浏览器访问 `192.168.4.1`
4. 配置 WiFi 和超时时间，保存
5. 设备重启并连接目标 WiFi

### 重新配网
长按 GPIO0 按钮 3 秒，进入配网模式

## Web API

| 端点 | 方法 | 说明 |
|------|------|------|
| `/` | GET | Web 配置页面 |
| `/api/config` | GET | 获取配置 |
| `/api/config` | POST | 保存配置 |
| `/api/nfc` | GET | 获取 NFC 状态 |
| `/api/nfc/add` | POST | 添加标签 |
| `/api/nfc/remove` | POST | 删除标签 |
| `/api/status` | GET | 获取设备状态 |
| `/api/reboot` | POST | 重启设备 |

## 配置存储

配置存储在 ESP32 NVS 分区：

| 键名 | 类型 | 说明 |
|------|------|------|
| wifi_ssid | String | WiFi名称 |
| wifi_pass | String | WiFi密码 |
| timeout_ms | ULong | 超时时间（毫秒） |
| notify_url | String | 通知URL |
| tag_0~tag_9 | String | 注册的NFC标签UID |

## 已知问题

- Web 页面有时不稳定（待优化）
- 按钮长按功能未完全测试

## License

MIT