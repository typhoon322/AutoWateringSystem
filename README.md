# AutoIrrigation — ESP32 多路自动浇花系统

GitHub: [typhoon322/AutoWateringSystem](https://github.com/typhoon322/AutoWateringSystem)

基于 **I2C 扩展子板** 的多分区自动灌溉（**v2.1：单泵 + 电磁阀**）：一块 ESP32-S3 管 1–10 盆。

## 硬件

| 组件 | 说明 |
|------|------|
| MCU | ESP32-S3（I2C GPIO8/9，0.96" SSD1306 状态屏） |
| 扩展板 | 3× ADS1115（湿度）+ 1× PCA9555（10 路阀） |
| 传感器 | 湿度探头 × 1–10 + 共用 YF-S201 流量计 |
| 执行 | 1 × 水泵 + **1–10 × 12 V 电磁阀** |
| 分区 | `zone_count` 1–10，单盆设 `set zones 1` 即可 |

### 浇水流程

开目标阀 → 等 500 ms → 开泵 → 定量停泵 → 关阀。多盆自动排队。

详细接线见 [docs/wiring.md](docs/wiring.md)，采购清单见 [docs/bom.md](docs/bom.md)。

## 文档

| 文档 | 说明 |
|------|------|
| [系统设计](docs/system-design.md) | 架构、状态机、安全、API 概要 |
| [开发文档](docs/development.md) | 编译烧录、CLI、Web API、标定、测试 |
| [使用说明书](docs/user-manual.md) | 组装、配网、日常操作、故障排查 |
| [文档-代码映射](docs/DOC_MAP.md) | 变更时需同步更新的文档对照表 |
| [项目状态 / 上下文摘要](docs/project-status.md) | 里程碑、裸板验证结果、待办、新 Chat 接续 |

## 快速开始

> **本机路径**：`~/ESP32/AutoIrrigationSystem/AutoIrrigationSystem`（原 `Untitled` 已重命名）。在 Cursor / VS Code 中请打开**含 `platformio.ini` 的这一层**目录。

### 环境

- [PlatformIO](https://platformio.org/)

### 编译与烧录

```bash
pio run -e esp32-s3-irrigation -t upload
pio device monitor -b 115200
```

### 串口 CLI（115200）

```
help / status / water 0 100
set zones 1          # 单盆模式
valve 0 / valve off  # 调试阀
save
```

完整命令见 [docs/development.md](docs/development.md)。

### Web 界面

1. 串口配置 WiFi 或连接 AP 热点
2. 浏览器打开 `http://<设备IP>/`
3. 查看湿度、手动浇水、修改阈值与体积（最多 10 盆）

## 项目结构

```
include/platform.h              # 板级入口
include/boards/board_io_map.h   # 扩展板 I2C 布局
include/boards/board_s3.h       # ESP32-S3 引脚
src/bus/                        # I2C 初始化
src/sensor/ads1115.*          # 湿度 ADC
src/actuator/pca9555.*        # 电磁阀扩展
src/control/                  # 灌溉控制器
src/web/                      # Web UI
docs/                         # 设计 / 开发 / 用户文档
```

## 文档同步

修改固件或硬件配置时，请对照 [docs/DOC_MAP.md](docs/DOC_MAP.md) 更新相关文档。Cursor 规则：`.cursor/rules/documentation-sync.mdc`。

## 参考

架构模式参考同目录兄弟项目 **TempControl**（ESP32 PlatformIO + Web UI + NVS）。
