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
| `include/boards/board_s3.h` | system-design, wiring, development, README, bom | ESP32-S3 引脚 |
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
| `src/ble/ble_link.*` | development, user-manual | BLE 协议、安卓 App 替代 Web 家庭页 |
| `src/cli/serial_cli.*` | development, user-manual | CLI 命令 |
| `src/display/display_driver.*` | user-manual | OLED 状态页 |
| `src/main.cpp` | development | 启动流程、采样周期 |
| `platformio.ini` | development, README, user-manual | env 名称、依赖库 |
| `docs/project-status.md` | — | 项目里程碑、上下文摘要（换目录/新 Chat 用） |
| `docs/bom.md` | wiring, user-manual | 元器件变更 |
| `docs/wiring.md` | user-manual, bom | 用户接线步骤 |
| `docs/engineering-wiring.md` | bom, wiring | 工程化布线定稿（汇流排/端子排/施工） |
| `docs/safety-checklist.md` | engineering-wiring, bom | 成品防护与电气安全施工清单 |

## 文档文件路径

| 简称 | 完整路径 |
|------|----------|
| system-design | `docs/system-design.md` |
| development | `docs/development.md` |
| user-manual | `docs/user-manual.md` |
| wiring | `docs/wiring.md` |
| bom | `docs/bom.md` |
| project-status | `docs/project-status.md` |
| README | `README.md` |

## 变更类型速查

| 变更类型 | 至少更新 |
|----------|----------|
| 引脚 / 接线 | wiring, system-design, board_io_map, board_s3 |
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
| 2026-08-02 | 新增 project-status.md（项目上下文摘要）；目录 Untitled→AutoIrrigationSystem |
| 2026-08-01 | C3 0.42 OLED 板：引脚改为 I2C 5/6、流量计 GPIO7；新增 board-esp32-c3-042-oled.md |
| 2026-08-18 | Web dashboard 联调测试增强：features 字段、状态展示、泵计时/ppl/安全测试/步骤引导 |
| 2026-08-18 | 继电器选型红线：3.3V MCU 不可直驱 5V/12V 线圈光耦模块；固件改"高阻关断+0V 吸合"（pca9555/valve/pump 驱动） |
| 2026-08-18 | 工程布线方案 v1.0 定稿（docs/engineering-wiring.md）：汇流排/端子排/湿度计线束/10 盆扩展 |
| 2026-08-22 | WebUI 家庭模式：`/` 家庭首页 + `/dev` 调试页、浇水历史（NVS 持久化）、浇水时间窗口（全局+每盆覆盖） |
| 2026-08-22 | 显示屏接入：ST7789+LVGL 3 屏中文界面（主界面/浇水/故障）、中文字库、引脚宏集中 board_s3.h |
| 2026-08-22 | 显示屏 v2：换 SH1106 128×64 I2C（共用总线 0x3C、LVGL 单色 1bpp、EC11+KEY0/KEY1），ST7789 方案作废 |
| 2026-08-22 | 安全施工清单（docs/safety-checklist.md）：续流二极管/保险丝/防水盒/三防漆/看门狗/接线红线 |
| 2026-10-02 | S3 状态屏改为 0.96" SSD1306 128×64；GPIO6 短按检测浇水，GPIO7 急停 |
| 2026-10-03 | BLE 外设（广播名 Langua）+ 安卓 App `android/` |
| 2026-10-03 | 停用 ESP32-C3 验证环境，固件与文档只保留 ESP32-S3 |
