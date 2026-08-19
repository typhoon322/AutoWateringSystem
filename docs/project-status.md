# 项目状态与上下文摘要

> 版本：2026-08-02 | 固件 v2.1 | 供换目录 / 新 Chat 时快速恢复上下文

## 1. 仓库与路径

| 项 | 值 |
|----|-----|
| GitHub | [typhoon322/AutoWateringSystem](https://github.com/typhoon322/AutoWateringSystem.git) |
| 本机项目根 | `~/ESP32/AutoIrrigationSystem/AutoIrrigationSystem` |
| 原目录名 | `Untitled`（已重命名，Cursor 旧 Chat 挂在旧路径下） |
| IDE 打开 | **含 `platformio.ini` 的这一层**，不要只开外层文件夹 |

```bash
cd ~/ESP32/AutoIrrigationSystem/AutoIrrigationSystem
~/.platformio/penv/bin/pio run -e esp32-c3-oled-test -t upload
```

## 2. 架构概要（v2.1）

- **单泵 + 多电磁阀**（1–10 区），共用 **YF-S201 流量计**
- **I2C 扩展子板**：3× ADS1115（湿度）+ 1× PCA9555（阀）
- **前期验证**：01Space ESP32-C3 0.42" OLED（`esp32-c3-oled-test`）
- **量产目标**：ESP32-S3（`esp32-s3-irrigation`），扩展板接线不变
- **交互**：串口 CLI + 内嵌 Web UI + 板载 OLED

浇水顺序：**开阀 → 500 ms → 开泵 → 定量停泵 → 关阀**。

## 3. 当前硬件进度

| 阶段 | 状态 |
|------|------|
| C3 0.42 OLED 裸板（仅 USB） | ✅ 已烧录验证 |
| 板载 OLED 显示 | ✅ U8g2 72×40 |
| WiFi 混合模式 AP+STA | ✅ 已连 `OneMore`，AP 同时开 |
| Web UI | ✅ `192.168.4.1` 或局域网 IP（2026-08-18 已增强联调测试页） |
| S3 量产板串口/Web 联调 | ✅ 固件跑通（Serial=UART0、WiFi 连 `ChinaUnicom-8DFA-2.4`、Web 192.168.0.112） |
| PCA9555 阀扩展板 | ✅ I2C 在线（0x20）；驱动逻辑已改"高阻关断+0V 吸合" |
| 泵/阀继电器 | ⏳ 待换 **3.3V 低电平触发**模块（5V/12V 线圈模块与 3.3V MCU 不兼容，见 [bom.md 红线](bom.md)） |
| ADS1115 湿度板 / 流量计 | ⏳ 待接线 |

### C3 板关键引脚（`include/boards/board_c3_oled.h`）

| 功能 | GPIO | 备注 |
|------|------|------|
| I2C SDA / SCL | **5 / 6** | 板载 OLED + 扩展板共用；**勿用排针 8/9** |
| 水泵继电器 | 3 | |
| 流量计 | **7** | 勿接 GPIO5（OLED SDA） |
| 状态 LED | 8 | 低电平亮 |

## 4. 已验证行为（裸板）

串口典型输出：

```
AutoIrrigation v2.1
Board: ESP32-C3-0.42-OLED
OLED: U8g2 72x40 initialized
WARN: no ADS1115 detected on I2C
WARN: PCA9555 valve expander not found
WiFi: enabled
AP: 192.168.4.1 (hybrid)
WiFi connecting to OneMore...
WiFi connected: 192.168.x.x
```

OLED 四行：状态 / Z0·Z1 湿度 / 泵阀 / 日流量·AP 指示。

## 5. 重要实现细节（避免重复踩坑）

### OLED

- 驱动：**U8g2** `U8G2_SSD1306_72X40_ER_F_HW_I2C`（与 01Space 官方例程一致）
- `display_driver.cpp` 必须在 `#if BOARD_HAS_OLED` **之前** `#include "config.h"`，否则整段驱动被编译跳过
- 初始化顺序：`irrigationI2cBegin()` → **先 OLED** → 再探测 ADS1115 / PCA9555
- 不要用 Adafruit SSD1306 128×64 + 偏移；不要用带 pin 参数的 U8g2 构造（曾导致卡死）

### WiFi（与 TempControl 对齐）

| 宏 | 值 |
|----|-----|
| `WIFI_SSID` | `OneMore` |
| `WIFI_PASS` | `onemore.2025` |
| `WIFI_HYBRID_MODE` | `1`（AP+STA 同时） |
| `WIFI_AP_SSID` / `PASS` | `ESP32-IRRIGATION` / `irrigate2026` |
| `RADIO_WIFI_DEFAULT_ENABLED` | `1` |

访问 Web：`http://<STA_IP>/` 或 `http://192.168.4.1/`。

若 NVS 里曾存 `wifi off`：串口 `wifi on` + `save`，或清 NVS 后重启。

### USB 串口

`platformio.ini` 中 C3 环境需：

```ini
-DARDUINO_USB_MODE=1
-DARDUINO_USB_CDC_ON_BOOT=1
```

## 6. 未提交改动（截至 2026-08-02）

本地 **master** 相对 `origin/master` 有未 push 工作区修改，主要包括：

| 类别 | 文件/目录 |
|------|-----------|
| OLED 驱动 | `src/display/`（新） |
| WiFi 混合模式 | `include/config.h`, `src/web/web_server.*`, `src/storage/settings_store.cpp` |
| 启动顺序 | `src/main.cpp` |
| 文档 | `README.md`, `docs/development.md`, `docs/user-manual.md`, `docs/board-esp32-c3-042-oled.md`, `docs/DOC_MAP.md` |
| 规则 | `.cursor/rules/*.mdc` |

远端最新 commit：`30debe8`（USB CDC）。**OLED + WiFi 混合模式尚未 commit/push。**

## 7. 待办（建议顺序）

1. [ ] 提交并 push OLED + WiFi 相关改动
2. [ ] 焊接/接入 I2C 扩展子板（SDA/SCL → GPIO5/6）
3. [ ] 接 1 路湿度 + 泵 + 阀 + 流量计，单区联调
4. [ ] 标定 `pulses_per_liter`、各 `volume_ml`（管长不等时逐区标）
5. [ ] 扩展至 2+ 区，验证队列灌溉
6. [ ] 量产板 ESP32-S3 同扩展板冒烟测试
7. [ ] **整合 PCB 设计**（量产整合板：S3 模块 + 3×ADS1115 + PCA9555 + 泵/阀驱动 + 电源一体，KiCad 出图打样；输入见联调定型的极性/电平/地址决策）

## 8. 文档索引

| 文档 | 用途 |
|------|------|
| [system-design.md](system-design.md) | 架构、状态机、安全 |
| [development.md](development.md) | 编译、CLI、API |
| [user-manual.md](user-manual.md) | 组装、配网、操作 |
| [board-esp32-c3-042-oled.md](board-esp32-c3-042-oled.md) | C3 小板引脚与 OLED |
| [wiring.md](wiring.md) / [bom.md](bom.md) | 接线与采购 |
| [DOC_MAP.md](DOC_MAP.md) | 改代码时需同步的文档 |

## 9. 给新 Chat 的提示词模板

复制以下内容到新对话即可快速接续：

```
项目：AutoIrrigation v2.1 自动灌溉（ESP32-C3 0.42 OLED 验证中）
路径：~/ESP32/AutoIrrigationSystem/AutoIrrigationSystem
请先读 docs/project-status.md，再继续：<你的任务>
```

---

*本文档随里程碑更新；结构性变更时同步 [DOC_MAP.md](DOC_MAP.md) 版本记录。*
