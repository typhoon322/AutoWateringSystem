# 接线说明 — I2C 扩展板 + 单泵多阀

## 架构

**一块 I2C 扩展子板** 接所有湿度探头与电磁阀；MCU 只接 I2C、泵、流量计、LED。

| 器件 | I2C 地址 | 用途 |
|------|----------|------|
| ADS1115 #0 | 0x48 | 湿度 AIN0–3 → 盆 0–3 |
| ADS1115 #1 | 0x49 | 湿度 AIN0–3 → 盆 4–7 |
| ADS1115 #2 | 0x4A | 湿度 AIN0–1 → 盆 8–9 |
| PCA9555 | 0x20 | 阀继电器 P0–P9 → 盆 0–9 |
| OLED（仅 C3 测试板） | 0x3C | 与扩展板共用 I2C 总线 |

布局定义见 [`include/boards/board_io_map.h`](../include/boards/board_io_map.h)。

## MCU 直连（因板型而异）

| 功能 | C3 OLED 测试 (`board_c3_oled.h`) | S3 量产 (`board_s3.h`) |
|------|----------------------------------|------------------------|
| I2C SDA / SCL | GPIO6 / GPIO7 | GPIO8 / GPIO9 |
| 水泵继电器 | GPIO3 | GPIO4 |
| 流量计 | GPIO5 | GPIO5 |
| RGB LED | GPIO8 | GPIO48 |

## 电容式土壤湿度

每盆一路接 ADS1115 单端输入（3.3 V 供电，AO 进 ADS1115，GND 共地）。

## 水路

```
┌────────┐   ┌────┐   ┌────────┐   ├─ [阀0] ─→ 盆0
│ 水源桶  │──→│ 泵  │──→│ YF-S201 │──┼─ [阀1] ─→ 盆1
└────────┘   └────┘   └────────┘   └─ … 最多 10 路
                      ↑ 流量计
                   继电器（MCU GPIO）
```

- 流量计在泵后，测量总流量；每 session 重置计数
- 单盆：`set zones 1`，只接阀0 + 探头0

## 电磁阀（每路）

12 V 常闭阀，PCA9555 对应位经 **5 V 继电器** 或 **MOSFET** 驱动。阀与泵 **必须共地**。

## 电源

```
12V ──→ 泵、流量计、各电磁阀
  │
  GND ──共地── ESP32 GND
USB ──→ ESP32（开发）
或 12V ──→ 降压 5V/1A ──→ ESP32 + 继电器 VCC
```

## 烧录

```bash
# 前期验证（C3 + OLED）
pio run -e esp32-c3-oled-test -t upload

# 量产（S3，扩展板不变）
pio run -e esp32-s3-irrigation -t upload

pio device monitor -b 115200
```

## 上电检查

1. 串口打印 `AutoIrrigation v2.1` 与 `Zones: N (max 10 valves, I2C expanders)`
2. 无 ADS1115/PCA9555 时会 WARN，接好扩展板后重启
3. `status` 显示各盆湿度
4. `valve 0` / `valve off` 调试阀
5. 通水后 `water 0 50` 定量测试
