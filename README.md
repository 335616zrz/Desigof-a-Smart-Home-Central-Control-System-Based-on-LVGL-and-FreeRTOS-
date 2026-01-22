<div align="center">

# 智能家居中控系统

**基于 ESP32-S3 + LVGL + FreeRTOS 的端-云-移动一体化解决方案**

![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.3.4-blue)
![LVGL](https://img.shields.io/badge/LVGL-v9.2.1-green)
![License](https://img.shields.io/badge/License-Academic-orange)
![Version](https://img.shields.io/badge/Version-0.0.2-red)

毕业设计 · 2026

</div>

---

## 项目简介

本项目以 ESP32-S3 触控中控终端为核心，配套 IoT 云平台、语音对话 Chatbot 服务、天气服务、音乐资源库、OTA 固件分发以及微信小程序，实现了完整的智能家居控制与交互方案。

---

## 效果展示

> 本仓库当前未提交运行截图（`docs/images/` 仅保留目录占位）。
> 如需展示，可自行补充截图文件：`device.jpg` / `ui-home.png` / `ui-music.png`。

---

## 功能亮点

| 特性             | 说明                                                                                       |
| ---------------- | ------------------------------------------------------------------------------------------ |
| **触摸中控终端** | ESP32-S3 + 3.5" LCD + LVGL 多页面 UI                                                       |
| **音乐播放**     | HTTP 流媒体 + SD 卡本地播放 + 频谱可视化                                                   |
| **语音交互**     | ASR (Paraformer 流式 + SenseVoice final) + TTS (CosyVoice) + DeepSeek 大模型               |
| **IoT 云平台**   | 设备管理 / 遥测上报 / 告警规则 / 数据可视化                                                |
| **微信小程序**   | 移动端入口，远程控制与监控                                                                 |
| **OTA 升级**     | HTTPS 双分区升级（回滚需启用 `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`）                     |
| **内存优化**     | PSRAM 深度优化（任务栈/缓冲区外置到 PSRAM，释放内部堆；以实际 monitor 为准）               |

---

## 系统架构

**三层架构**：微信小程序 ↔ 云平台（Spring Boot + EMQX + 微服务）↔ ESP32-S3 终端

- **移动端**：微信小程序（uni-app X）- 远程控制与监控
- **云平台**：Docker Compose 部署
  - 后端：Spring Boot 3.2.5 + MySQL 8 + Redis 7.2
  - 消息：EMQX 5.6 (MQTT Broker)
  - 监控：Prometheus + Grafana
  - 微服务：Chatbot (语音对话) / Weather Service (天气代理)
- **终端设备**：ESP32-S3 (自研硬件)
  - UI：LVGL 9.2.1 多页面触控界面
  - 音频：HTTP 流媒体 + SD 卡播放
  - 语音：WebSocket 实时 ASR + TTS
  - 通信：MQTT 遥测上报 + HTTPS OTA 升级

> 架构图预留：`docs/images/architecture.png`（仓库暂未提供）

---

## 硬件配置

| 组件     | 型号          | 接口          | 说明           |
| -------- | ------------- | ------------- | -------------- |
| 主控     | ESP32-S3-N16R8| -             | 16MB Flash + 8MB PSRAM |
| 屏幕     | ST7796        | SPI@80MHz     | 480×320 横屏   |
| 触摸     | FT6336        | I2C@400kHz    | 电容触摸       |
| 音频输出 | MAX98357A     | I2S           | D类功放        |
| 麦克风   | INMP441       | I2S           | MEMS 数字麦    |
| 温湿度   | SHT40         | I2C           | 高精度传感器   |
| RGB LED  | WS2812B       | RMT (GPIO48)  | 夜灯/告警指示  |
| 存储     | SD Card       | SPI           | 本地音乐/日志  |

---

## 目录结构

```text
.
├── hardware_part/                    # 硬件设计 (立创 EDA / Altium)
├── software_part/                    # ESP-IDF 固件工程
│   ├── main/                         # 主程序入口
│   ├── components/
│   │   ├── 01_application/           # 应用层 (UI / 音乐 / 语音)
│   │   ├── 02_middleware/            # 中间件 (播放器 / MQTT / OTA / ASR)
│   │   ├── 03_drivers/               # 驱动层 (LCD / 触摸 / 传感器)
│   │   ├── 04_hardware/              # 硬件抽象层 (board_config)
│   │   └── common/                   # 公共工具 (mem_utils / error_utils)
│   ├── services/
│   │   ├── Chatbot/                  # 语音对话服务 (Node.js + Python)
│   │   ├── IoT_cloud_platform/       # IoT 云平台 (Spring Boot + Vue)
│   │   ├── Music_Server/             # 音乐文件服务 (Caddy 托管)
│   │   ├── weather-service/          # 天气代理服务 (Node.js)
│   │   ├── OTA/                      # 固件分发 (HTTPS)
│   │   └── WeChat_Mini_Program/      # 微信小程序 (uni-app X)
│   ├── credentials.json.example      # 凭据模板
│   └── setup_credentials.sh          # 凭据初始化脚本
├── docs/                             # 详细文档
└── tools/                            # 工具脚本 (安全检查 / Git Hook)
```

---

## 内存优化（PSRAM）

针对 ESP32-S3 内存限制，项目进行了两阶段优化：

| 阶段    | 优化内容                                                                                                                              | 效果                                       |
| ------- | ------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------ |
| Phase 1 | WebSocket/TLS 缓冲区、AEC 参考缓冲区迁移至 PSRAM                                                                                      | 内部堆占用降低（以实际 monitor 为准）      |
| Phase 2 | 任务栈迁移至 PSRAM（共 19KB：music 6KB / ws2812 3KB / env_sht40 3KB / splash_conn 4KB / wifi_stop 3KB）                               | 理论释放约 19KB 内部内存（启用相关 Kconfig 后生效） |

**示例日志**（数值会随配置/运行时变化，以实际 monitor 输出为准）：

```text
W (xxx) OTA_UPDATE: [OTA_DIAG] Before esp_https_ota_begin:
W (xxx) OTA_UPDATE:   Internal free=<bytes> largest=<bytes>
```

> 详细配置说明请参考 [DEPLOYMENT.md - PSRAM 配置](docs/DEPLOYMENT.md#step-6-psram-任务栈配置可选)

---

## 快速开始

### 1. 凭据配置

**credentials.json 结构说明**：以 `credentials.json.example` 为准（分为 `chatbot_service` / `weather_service` / `iot_cloud_platform` 三部分）。

```json
{
  "chatbot_service": {
    "deepseek_api_key": "YOUR_DEEPSEEK_API_KEY_HERE",
    "cosyvoice_api_key": "YOUR_COSYVOICE_API_KEY_HERE",
    "port": 3333,
    "https_enabled": true,
    "https_key_path": "certs/servers.local.key.pem",
    "https_cert_path": "certs/servers.local.cert.pem"
  },
  "weather_service": {
    "amap_api_key": "YOUR_AMAP_API_KEY_HERE",
    "port": 3001
  },
  "iot_cloud_platform": {
    "mysql": {
      "root_password": "CHANGE_ME_STRONG_PASSWORD",
      "password": "CHANGE_ME_STRONG_PASSWORD"
    },
    "emqx": { "dashboard_password": "CHANGE_ME_STRONG_PASSWORD" },
    "backend": { "jwt_secret": "CHANGE_ME_TO_RANDOM_64_CHAR_STRING_FOR_JWT_SIGNING" },
    "admin": { "password": "CHANGE_ME_STRONG_PASSWORD" },
    "grafana": { "admin_password": "CHANGE_ME_STRONG_PASSWORD" }
  }
}
```

**初始化步骤**：

```bash
cd software_part

# 复制模板并填入真实密钥
cp credentials.json.example credentials.json
vim credentials.json  # 或使用其他编辑器

# 生成各服务的 .env 文件
./setup_credentials.sh
```

脚本会生成：

- `services/Chatbot/.env`
- `services/weather-service/.env`
- `services/IoT_cloud_platform/deploy/.env`

> **注意**：`credentials.json` 和所有 `.env` 文件已加入 `.gitignore`，请勿提交到 Git。

### 2. 固件编译与烧录

```bash
cd software_part

# 设置 ESP-IDF 环境
source ~/esp/esp-idf/export.sh
export ADF_PATH=~/esp/esp-adf

# 编译
idf.py build

# 烧录
idf.py -p /dev/ttyUSB0 flash monitor
```

### 3. IoT 云平台部署

```bash
cd software_part/services/IoT_cloud_platform/deploy
docker compose up -d --build
```

> **完整部署指南**：[docs/DEPLOYMENT.md](docs/DEPLOYMENT.md)

---

## 服务端口速查

| 端口  | 服务            | 说明                    |
| ----- | --------------- | ----------------------- |
| 8088  | Nginx Gateway   | Web 前端入口            |
| 8080  | Spring Boot     | 后端 API                |
| 1883  | EMQX            | MQTT Broker             |
| 18083 | EMQX Dashboard  | MQTT 管理界面           |
| 3000  | Grafana         | 监控可视化              |
| 3333  | Chatbot         | 语音对话服务 (HTTPS)    |
| 3001  | Weather Service | 天气代理                |
| 443   | Caddy           | HTTPS 主入口 (可选)     |

---

## 技术栈总览

| 层级   | 技术                                                                                  |
| ------ | ------------------------------------------------------------------------------------- |
| 嵌入式 | ESP-IDF 5.3.4 / FreeRTOS / LVGL 9.2.1 / ESP-ADF                                       |
| 后端   | Spring Boot 3.2.5 / MyBatis / EMQX 5.6 / MySQL 8 / Redis 7.2                          |
| 前端   | Vue 3.4 / Vite 5 / Element Plus / ECharts                                             |
| 移动端 | uni-app X (微信小程序)                                                                |
| AI/语音| DeepSeek API / Paraformer (流式 ASR) / SenseVoice (final) / CosyVoice (TTS) / RNNoise |
| 运维   | Docker Compose / Nginx / Caddy / PM2 / Prometheus / Grafana                           |

---

## 详细文档

| 文档                                  | 说明                                             |
| ------------------------------------- | ------------------------------------------------ |
| [DEPLOYMENT.md](docs/DEPLOYMENT.md)   | 完整环境部署指南（ESP-IDF / PM2 / Caddy / Docker）|
| [SECURITY.md](docs/SECURITY.md)       | 安全扫描报告 + 敏感文件清单                      |

---

## 安全声明

本项目已按 [docs/SECURITY.md](docs/SECURITY.md) 中的可复现流程进行扫描（敏感文件检查 + 关键字/模式扫描），在扫描范围内未发现明显恶意代码特征。

> **完整安全报告**：[docs/SECURITY.md](docs/SECURITY.md)

**重要**：请勿将以下文件提交到 Git：

- `software_part/credentials.json` - 主凭据文件（API Keys、数据库密码等）
- `**/*.env` - 环境变量文件（包含各服务口令/secret）
- `software_part/sdkconfig` - ESP-IDF 实际生效配置（可能包含 Wi‑Fi/MQTT 明文）
- `**/*.pem` / `**/*.key` / `**/*.crt` - 证书与私钥

```bash
# 安装 Git 安全检查钩子
./tools/install_hooks.sh
```

ESP-IDF 配置建议：

- 仓库仅提交 `software_part/sdkconfig.defaults` 作为"基线配置"
- 本机实际使用的 `software_part/sdkconfig` 不要提交（已在 `.gitignore` 与 hook 中阻止）
- 更新基线配置：在 `software_part/` 下执行 `idf.py save-defconfig`（会生成/覆盖 `sdkconfig.defaults`）

---

## 许可证

本项目仅供学习与毕业设计使用。

---

**作者**：zrz
**版本**：0.0.2
