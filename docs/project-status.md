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
~/.platformio/penv/bin/pio run -e esp32-s3-irrigation -t upload
```

## 2. 架构概要（v2.1）

- **单泵 + 多电磁阀**（1–10 区），共用流量计
- **I2C 扩展子板**：3× ADS1115（湿度）+ 1× PCA9555（阀）
- **主控**：ESP32-S3（`esp32-s3-irrigation`），I2C GPIO8/9
- **交互**：0.96" SSD1306 状态屏 + 动作键/急停键 + 串口 CLI + Web UI + 蓝牙安卓 App

浇水顺序：**开阀 → 500 ms → 开泵 → 定量停泵 → 关阀**。

## 3. 当前硬件进度

| 阶段 | 状态 |
|------|------|
| S3 串口/Web 联调 | ✅ 固件跑通（Serial=UART0、WiFi 连 `ChinaUnicom-8DFA-2.4`、Web 192.168.0.112） |
| PCA9555 阀扩展板 | ✅ I2C 在线（0x20）；驱动逻辑已改"高阻关断+0V 吸合" |
| 泵/阀继电器 | ⏳ 待换 **3.3V 低电平触发**模块（5V/12V 线圈模块与 3.3V MCU 不兼容，见 [bom.md 红线](bom.md)） |
| ADS1115 湿度板 / 流量计 | ⏳ 待接线 |
| S3 状态屏 + 两个自复键 | 🔧 固件已接（SSD1306 128×64 @ GPIO8/9；GPIO6 动作、GPIO7 急停），待装机点亮 |
| 安卓 App（BLE 替代家庭页） | 🔧 固件广播名 `Langua`，工程在 `android/`，待手机联调 |

### S3 关键引脚（`include/boards/board_s3.h`）

| 功能 | GPIO | 备注 |
|------|------|------|
| I2C SDA / SCL | **8 / 9** | 扩展板 + 0.96" SSD1306 |
| 水泵继电器 | 4 | 低电平触发 |
| 流量计 | 5 | |
| 动作键 / 急停键 | 6 / 7 | 自复，另一端接 GND |
| 状态 LED | 48 | |

## 4. 已验证行为

串口典型输出：

```
AutoIrrigation v2.1
Board: ESP32-S3-Irrigation
WiFi: enabled
AP: 192.168.4.1 (hybrid)
WiFi connected: 192.168.x.x
```

状态屏：旋转 180°。第一行显示时间；未对时显示「请连手机蓝牙对时」。空闲时一页 3 盆，多于 3 盆每 10 秒翻页；浇水等工作中显示 `1# 正在浇水作业`。

## 5. 重要实现细节（避免重复踩坑）

### 状态屏

- 驱动：**U8g2** `U8G2_SSD1306_128X64_NONAME_F_HW_I2C`，地址 0x3C，I2C 100 kHz
- 初始化在 ADS/PCA 之后，失败会重试

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

## 6. 未提交改动（截至 2026-08-02）

本地 **master** 相对 `origin/master` 有未 push 工作区修改，主要包括：

| 类别 | 文件/目录 |
|------|-----------|
| OLED 驱动 | `src/display/`（新） |
| WiFi 混合模式 | `include/config.h`, `src/web/web_server.*`, `src/storage/settings_store.cpp` |
| 启动顺序 | `src/main.cpp` |
| 文档 | `README.md`, `docs/development.md`, `docs/user-manual.md`, `docs/DOC_MAP.md` |
| 规则 | `.cursor/rules/*.mdc` |

远端最新 commit：`30debe8`（USB CDC）。**OLED + WiFi 混合模式尚未 commit/push。**

## 7. 待办（建议顺序）

1. [ ] 提交并 push OLED + WiFi 相关改动
2. [ ] 焊接/接入 I2C 扩展子板（SDA/SCL → GPIO8/9）
3. [ ] 接 1 路湿度 + 泵 + 阀 + 流量计，单区联调
4. [ ] 标定 `pulses_per_liter`、各 `volume_ml`（管长不等时逐区标）
5. [ ] 扩展至 2+ 区，验证队列灌溉
6. [x] 主控定为 ESP32-S3（`esp32-s3-irrigation`）
7. [ ] **整合 PCB 设计**（量产整合板：S3 模块 + 3×ADS1115 + PCA9555 + 泵/阀驱动 + 电源一体，KiCad 出图打样；输入见联调定型的极性/电平/地址决策）
8. [x] **Web UI 家庭模式**（`/` 家庭首页 + `/dev` 调试页 + 浇水历史 NVS 持久化 + 浇水时间窗口；代码完成待硬件验证）
9. [x] **显示屏接入 v2**（SH1106 128×64 I2C + LVGL；代码在，`BOARD_HAS_LVGL=0` 未编入当前固件）
9b. [x] **0.96" SSD1306 状态屏 + 动作键/急停键**（2026-10-02，代码完成，待硬件点亮）
10. [ ] **固件看门狗 WDT**（esp_task_wdt，卡死自动复位；见 [safety-checklist.md](safety-checklist.md) §4）
11. [ ] **防水盒装机**（按 [safety-checklist.md](safety-checklist.md) 施工：保险丝/续流二极管/格兰头/三防漆/透气阀/温升测试）

## 8. 文档索引

| 文档 | 用途 |
|------|------|
| [system-design.md](system-design.md) | 架构、状态机、安全 |
| [development.md](development.md) | 编译、CLI、API |
| [user-manual.md](user-manual.md) | 组装、配网、操作 |
| [wiring.md](wiring.md) / [bom.md](bom.md) | 接线与采购 |
| [DOC_MAP.md](DOC_MAP.md) | 改代码时需同步的文档 |

## 9. 给新 Chat 的提示词模板

复制以下内容到新对话即可快速接续：

```
项目：AutoIrrigation v2.1 自动灌溉（ESP32-S3）
路径：~/ESP32/AutoIrrigationSystem/AutoIrrigationSystem
请先读 docs/project-status.md，再继续：<你的任务>
```

---

*本文档随里程碑更新；结构性变更时同步 [DOC_MAP.md](DOC_MAP.md) 版本记录。*
