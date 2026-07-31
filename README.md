# AutoIrrigation — ESP32-C3 多路自动浇花系统

GitHub: [typhoon322/AutoWateringSystem](https://github.com/typhoon322/AutoWateringSystem)

基于 ESP32-C3 的多分区自动灌溉系统：电容式土壤湿度检测 + YF-S201 流量计定量浇水 + WiFi 本地 Web 监控。

## 硬件

| 组件 | 说明 |
|------|------|
| MCU | ESP32-C3-DevKitM-1（板载 RGB LED） |
| 传感器 | 电容式土壤湿度 × 2，YF-S201 流量计 × 1 |
| 执行 | 12 V 微型水泵 + 5 V 继电器 |
| 分区 | 初版 2 路，单泵顺序灌溉，可扩展 4 路 |

详细接线见 [docs/wiring.md](docs/wiring.md)，采购清单见 [docs/bom.md](docs/bom.md)。

## 文档

| 文档 | 说明 |
|------|------|
| [系统设计](docs/system-design.md) | 架构、状态机、安全、API 概要 |
| [开发文档](docs/development.md) | 编译烧录、CLI、Web API、标定、测试 |
| [使用说明书](docs/user-manual.md) | 组装、配网、日常操作、故障排查 |
| [文档-代码映射](docs/DOC_MAP.md) | 变更时需同步更新的文档对照表 |

## 快速开始

### 环境

- [PlatformIO](https://platformio.org/)

### 编译与烧录

```bash
pio run -e esp32-c3-irrigation -t upload
pio device monitor -b 115200
```

### 串口 CLI（115200）

```
help          命令列表
status        各分区湿度与系统状态
water 0 100   Zone 0 浇水 100 mL
wifi ssid X   设置 WiFi
wifi on       开启 WiFi
save          保存配置
```

完整命令见 [docs/development.md](docs/development.md)。

### Web 界面

1. 串口配置 WiFi 或连接 AP 热点
2. 浏览器打开 `http://<设备IP>/`
3. 查看湿度、手动浇水、修改阈值与体积

## 项目结构

```
include/boards/board_c3.h   # 引脚
src/sensor/                 # 湿度、流量计
src/actuator/               # 水泵
src/control/                # 灌溉控制器
src/web/                    # Web UI
docs/                       # 设计 / 开发 / 用户文档
```

## 文档同步

修改固件或硬件配置时，请对照 [docs/DOC_MAP.md](docs/DOC_MAP.md) 更新相关文档。Cursor 规则：`.cursor/rules/documentation-sync.mdc`。

## 参考

架构模式参考同目录兄弟项目 **TempControl**（ESP32-C3 PlatformIO + Web UI + NVS）。
