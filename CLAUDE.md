# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

ESP32-S3 时间管控器，用于管理Nintendo Switch游戏机使用时间。
- **核心功能**：检测NFC标签，判断Switch是否在盒内，超时报警
- **V1.2版本**：WiFi配网 + NFC验证 + 离开计时 + 30分钟超时报警 + NVS异常检测

## 开发环境

### PlatformIO（推荐）
```bash
pio run              # 编译
pio run --target upload  # 上传固件
pio run --monitor    # 串口监视器（115200波特率）
```

## 硬件接线

| ESP32引脚 | 功能 | 备注 |
|-----------|------|------|
| GPIO1 | NFC SDA | I2C数据 |
| GPIO2 | NFC SCL | I2C时钟 |
| GPIO4 | NFC IRQ | 中断信号 |
| GPIO12 | LED 红色 | 串联220Ω电阻 |
| GPIO13 | LED 绿色 | 串联220Ω电阻 |
| GPIO14 | 蜂鸣器 | 有源5V，低电平触发 |
| GPIO0 | 配置按钮 | 内部上拉，长按3秒触发 |

### LED说明
- 共阳极双色LED
- 高电平点亮对应颜色

### LED状态
| 状态 | LED行为 |
|------|---------|
| 配网模式 | 红绿灯交替闪烁 |
| NFC在位+UID匹配 | 绿色常亮 |
| NFC离开或UID不匹配 | 红色常亮 |
| 超时报警 | 红色快速闪烁 |

## 代码架构（模块化）

```
src/
├── main.cpp           # 主程序，整合所有模块
├── nfc_module.cpp/h   # NFC中断驱动+防抖
├── led_module.cpp/h   # LED控制
├── buzzer_module.cpp/h # 蜂鸣器控制
├── wifi_module.cpp/h  # WiFi配网+Web服务器

platformio.ini        # 构建配置
```

### WiFi配网模块
- AP热点: TimeControl (密码: 12345678)
- 支持WiFi热点自动扫描
- 支持隐藏SSID手动输入
- NTP对时（阿里云服务器）

### NFC模块
- 中断驱动，GPIO4下降沿触发
- 防抖机制: 50ms初始延迟 + 1000ms冷却

### 计时器模块
- 离开计时，断电续算
- 跨天自动重置
- 30分钟超时报警

## NVS存储

### wifi_cfg 命名空间
| 键名 | 类型 | 说明 |
|------|------|------|
| ssid | String | WiFi名称 |
| pass | String | WiFi密码 |
| nfc_uid | String | 注册的NFC UID |

### timer 命名空间
| 键名 | 类型 | 说明 |
|------|------|------|
| absent_duration | ULong | 累计离开时长(毫秒) |
| absent_start | ULong | 本次离开开始时间 |
| last_day | Int | 当天日期编码 |
| last_millis_day | ULong | millis天数(备用) |

## 关键实现细节

1. **计时器溢出处理**：`safeMillisDiff()` 处理millis()约49.7天溢出的情况

2. **跨天检测**：双重方案
   - NTP已同步时使用正常日期
   - NTP未同步时使用millis()天数作为备用

3. **按键中断**：ISR中只设置标志，实际处理在loop()中执行

4. **NFC验证**：NVS中无UID时不校验，有UID时验证匹配

## 验证清单

- [x] WiFi配网功能正常
- [x] 按键长按3秒进入配网模式
- [x] 短按注册NFC UID
- [x] NFC验证逻辑正确
- [x] 离开计时准确
- [x] 30分钟超时报警
- [x] 跨天自动重置
- [x] 断电后继续计时