# ESP32-C3 0.42" OLED 开发板说明

> 适用板：01Space / Spotpear 等 **ESP32-C3FH4/FN4 + 0.42 寸 OLED** 一体小板（4 MB Flash）

## 官方资料

| 资源 | 链接 |
|------|------|
| GitHub（原理图、例程、MicroPython） | [01Space/ESP32-C3-0.42LCD](https://github.com/01Space/ESP32-C3-0.42LCD) |
| 原理图 PDF | [ESP32-C3-0.42OED Schematic.pdf](https://github.com/01Space/ESP32-C3-0.42LCD/blob/main/Schematic/ESP32-C3-0.42OED%20Schematic.pdf) |
| Zephyr 板级文档 | [ESP32C3 0.42 OLED](https://docs.zephyrproject.org/latest/boards/01space/esp32c3_042_oled/doc/index.html) |
| 引脚参考 | [espboards.dev](https://www.espboards.dev/esp32/esp32-c3-oled-042/) |
| 乐鑫 C3 芯片 | [ESP32-C3 文档](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/index.html) |

## 板载资源

| 项目 | 规格 |
|------|------|
| MCU | ESP32-C3FH4 / FN4，RISC-V 160 MHz |
| Flash | 4 MB |
| 屏幕 | 0.42" OLED，SSD1306，I2C **0x3C** |
| USB | Type-C，板载 USB 串口 |
| 按键 | BOOT（GPIO9）、RESET |
| LED | GPIO8，**低电平亮** |
| RGB（部分批次） | WS2812 @ GPIO2（本固件未使用） |

## 与本项目引脚对应

固件定义见 [`include/boards/board_c3_oled.h`](../include/boards/board_c3_oled.h)。

| 功能 | GPIO | 说明 |
|------|------|------|
| I2C SDA | **5** | 板载 OLED + 扩展板 ADS1115 / PCA9555 **共用** |
| I2C SCL | **6** | 同上 |
| 水泵继电器 | **3** | MCU 直连 |
| 流量计脉冲 | **7** | MCU 直连；**勿接 GPIO5**（OLED SDA） |
| 状态 LED | **8** | 板载 LED，低电平亮 |

### I2C 设备地址（同一条总线）

| 设备 | 地址 |
|------|------|
| 板载 OLED | 0x3C |
| ADS1115 #0 | 0x48 |
| ADS1115 #1 | 0x49 |
| ADS1115 #2 | 0x4A |
| PCA9555 阀扩展 | 0x20 |

扩展板 SDA/SCL 接到 **GPIO5 / GPIO6**，与 OLED 并联即可，注意 3.3 V 共地。

### 空闲 GPIO（可扩展）

| GPIO | 注意 |
|------|------|
| 0, 1, 3 | 3 已用作泵；0/1 可作 ADC |
| 2 | 部分板子接 WS2812 |
| 4, 7 | 7 已用作流量计 |
| 10 | 通用 IO |
| 9 | BOOT 键，勿强下拉 |
| 20, 21 | USB 串口，避免占用 |

## 仅泵联调（暂无阀 / 流量计）

当前 `esp32-c3-oled-test` 编译选项默认关闭阀与流量计（见 `platformio.ini` 中 `IRRIGATION_HAS_*=0`）。

| 接线 | 说明 |
|------|------|
| 水泵继电器 IN | **GPIO3** |
| 继电器 VCC/GND | 5 V / GND（与 ESP32 共地） |
| 泵 12 V | 经继电器常开触点 |

**串口测试：**

```
pump on          # 直接开泵（调试）
pump off
water 0 100      # 按时间估算约 100 ml（默认 ~10 ml/s，约 10 s）
stop             # 急停
set zones 1      # 单盆模式
```

上电应看到 `INFO: valves disabled` / `INFO: flow meter disabled`。装好阀或流量计后，把 `platformio.ini` 里对应宏改回 `1` 并重新烧录。

---

```bash
pio run -e esp32-c3-oled-test -t upload
pio device monitor -b 115200
```

PlatformIO 使用 `board = esp32-c3-devkitm-1`（与 C3FH4 4 MB 兼容）。

上电应显示：

```
AutoIrrigation v2.1
Board: ESP32-C3-0.42-OLED
```

## OLED 显示（可选，v2.2+）

板载屏为 **72×40 可视** 的 SSD1306（128×64 缓冲）。若后续加本地 UI，需：

```cpp
Wire.begin(5, 6);
// U8g2 示例：U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 6, 5);
// 0.42 寸常需 xOffset≈30, yOffset≈12
```

当前 v2.1 固件 **未驱动 OLED**，以 Web + 串口为主；屏不影响灌溉功能。

## 接线示意

```
ESP32-C3 0.42 OLED                I2C 扩展子板
+-------------------+              +------------------+
| GPIO5 SDA --------+--------------| SDA (OLED+ADS+PCA)
| GPIO6 SCL --------+--------------| SCL
| 3V3 / GND --------+--------------| VCC / GND
| GPIO3 -------------+-> 水泵继电器 |
| GPIO7 -------------+-> 流量计 SIG |
+-------------------+              +------------------+
```

完整水路见 [wiring.md](wiring.md)。
