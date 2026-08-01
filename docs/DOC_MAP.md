# 文档-代码映射表（DOC_MAP）

> 修改代码或硬件定义时，必须同步更新本表所列文档。

## 使用方式

1. 完成代码变更
2. 在本表查找对应「代码路径」
3. 更新所有列出的文档
4. 若新增模块，在本表追加一行

## 映射表

| 代码路径 | 需更新的文档 | 更新内容 |
|----------|-------------|----------|
| `include/platform.h` | system-design, development, README | MCU/板型策略 |
| `include/boards/board_io_map.h` | system-design, wiring, bom, development | I2C 地址、通道/位映射 |
| `include/boards/board_c3_oled.h` | board-esp32-c3-042-oled, system-design, wiring, development, bom, README |
| `include/boards/board_s3.h` | system-design, wiring, development, README, bom | S3 量产 MCU 引脚 |
| `include/config.h` | system-design, development, user-manual | 默认值、宏常量 |
| `include/types/zone_config.h` | system-design, development | 数据结构、字段 |
| `src/bus/i2c_bus.*` | development, wiring | I2C 初始化 |
| `src/sensor/ads1115.*` | system-design, development, bom | ADS1115 驱动、湿度 ADC |
| `src/sensor/moisture_sensor.*` | system-design, development | 校准、百分比 |
| `src/sensor/flow_meter.*` | system-design, development, user-manual | 脉冲系数、标定步骤 |
| `src/actuator/pca9555.*` | wiring, bom, development | PCA9555 阀驱动 |
| `src/actuator/pump_driver.*` | wiring, development, bom | 水泵继电器 |
| `src/actuator/valve_driver.*` | wiring, development, system-design | 电磁阀互锁 |
| `src/control/zone_manager.*` | system-design, development | 分区管理 |
| `src/control/irrigation_controller.*` | system-design, development, user-manual | 状态机、浇水模式 |
| `src/safety/safety_monitor.*` | system-design, user-manual | 安全策略、故障码 |
| `src/storage/settings_store.*` | development | NVS 键名 |
| `src/web/web_server.*` | development, user-manual | Web API、界面操作 |
| `src/cli/serial_cli.*` | development, user-manual | CLI 命令 |
| `src/ota/firmware_ota.*` | development | OTA 流程 |
| `src/main.cpp` | development | 启动流程、采样周期 |
| `platformio.ini` | development, README, user-manual | env 名称、依赖库 |
| `docs/bom.md` | wiring, user-manual | 元器件变更 |
| `docs/wiring.md` | user-manual, bom | 用户接线步骤 |

## 文档文件路径

| 简称 | 完整路径 |
|------|----------|
| system-design | `docs/system-design.md` |
| development | `docs/development.md` |
| user-manual | `docs/user-manual.md` |
| wiring | `docs/wiring.md` |
| bom | `docs/bom.md` |
| board-c3 | `docs/board-esp32-c3-042-oled.md` |
| README | `README.md` |

## 变更类型速查

| 变更类型 | 至少更新 |
|----------|----------|
| 引脚 / 接线 | wiring, system-design, board_io_map, board_c3_oled / board_s3 |
| I2C 扩展布局 | board_io_map, bom, wiring, system-design |
| 新增 API / CLI | development, user-manual（若用户可见） |
| 安全逻辑 | system-design, user-manual 故障表 |
| 默认值 / 配置项 | config.h, development, user-manual |
| 新硬件器件 | bom, wiring, system-design |

## 版本记录

| 日期 | 变更 |
|------|------|
| 2026-08-01 | 初版创建，v1.0 文档与代码基线 |
| 2026-08-01 | v2.1：I2C 扩展板（10 盆）、C3 验证 / S3 量产、更新 BOM 与映射 |
| 2026-08-01 | BOM v2.1：电磁阀两通常闭、流量计/止回阀/分水器；管长标定文档 |
| 2026-08-01 | C3 0.42 OLED 板：引脚改为 I2C 5/6、流量计 GPIO7；新增 board-esp32-c3-042-oled.md |
