# 完整环境部署指南

> 本文档提供从零开始的完整部署步骤，适用于工程迁移或新机器配置。

## 目录

- [Step 1: 创建目录结构](#step-1-创建目录结构)
- [Step 2: 克隆工程与 ESP 组件](#step-2-克隆工程与-esp-组件)
- [Step 3: 安装 ESP-IDF 工具链](#step-3-安装-esp-idf-工具链)
- [Step 4: 配置 mDNS (Avahi)](#step-4-配置-mdns-avahi)
- [Step 5: 配置 PM2（Node.js 进程管理）](#step-5-配置-pm2nodejs-进程管理)
- [Step 6: PSRAM 任务栈配置](#step-6-psram-任务栈配置可选)
- [Step 7: 配置 Caddy（Web 服务器）](#step-7-配置-caddyweb-服务器--反向代理)
- [Step 8: 配置 IoT 云平台 (Docker)](#step-8-配置-iot-云平台-docker)

---

## Step 1: 创建目录结构

```bash
mkdir -p ~/projects ~/esp
```

---

## Step 2: 克隆工程与 ESP 组件

### 2.1 克隆本工程

```bash
cd ~/projects
git clone <your-repo-url> 2026_undergraduate_graduation_project
```

### 2.2 克隆 ESP-IDF v5.3.4（与工程锁定版本一致）

本工程通过 ESP Component Manager 锁定了 ESP-IDF 版本：见 `software_part/dependencies.lock` 中的 `idf: version: 5.3.4`。

```bash
cd ~/esp

# （可选）Gitee 工具（用于加速子模块下载）
git clone https://gitee.com/EspressifSystems/esp-gitee-tools.git

# 克隆 ESP-IDF
git clone https://gitee.com/EspressifSystems/esp-idf.git
cd esp-idf

# 切换到 v5.3.4 标签（推荐）
git checkout v5.3.4

# 使用 Gitee 镜像更新子模块
# 若你未安装 esp-gitee-tools，可改用：git submodule update --init --recursive
../esp-gitee-tools/submodule-update.sh .

# 验证版本（可选）
git describe --tags --always
```

### 2.3 克隆 ESP-ADF（本工程仅用到 3 个组件）

本工程固件侧只引入 ESP-ADF 的 3 个组件：`audio_pipeline`、`audio_sal`、`esp-adf-libs`（见 `software_part/CMakeLists.txt`）。

```bash
cd ~/esp
git clone https://gitee.com/EspressifSystems/esp-adf.git
cd esp-adf

# 初始化必要子模块（esp-adf-libs）
git submodule update --init components/esp-adf-libs
```

### 2.4 获取工程组件依赖（ESP Component Manager）

```bash
# 注意：需要先完成 Step 3（安装并 source ESP-IDF 环境）后再执行
cd ~/projects/2026_undergraduate_graduation_project/software_part

# 第一次构建/配置时，ESP Component Manager 会按 idf_component.yml + dependencies.lock
# 自动下载依赖到 software_part/managed_components/
idf.py build
```

> 说明：本工程使用的 `espressif/esp-sr` 来自 ESP Component Manager（见 `software_part/dependencies.lock`），
> 不需要在 ESP-ADF 仓库中初始化 `components/esp-sr` 子模块。

**验证（可选）**：

- `software_part/dependencies.lock` 中应包含 `idf: version: 5.3.4`、`lvgl/lvgl: version: 9.2.1`
- `software_part/managed_components/` 下能看到对应依赖目录（首次构建后生成）

---

## Step 3: 安装 ESP-IDF 工具链

参考官方文档：[ESP-IDF Linux/macOS 安装指南](https://docs.espressif.com/projects/esp-idf/zh_CN/v5.3.4/esp32s3/get-started/linux-macos-setup.html)

```bash
cd ~/esp/esp-idf
./install.sh esp32s3

# 设置环境变量（每次开发前执行，或添加到 ~/.bashrc）
source ~/esp/esp-idf/export.sh
```

---

## Step 4: 配置 mDNS (Avahi)

mDNS 用于局域网内通过 `servers.local` 域名访问服务。

```bash
# 安装 Avahi
sudo apt update
sudo apt install -y avahi-daemon avahi-utils libnss-mdns

# 查看网卡名称（找到有 192.x.x.x 地址的网卡，如 eth0）
ip -brief addr

# 备份并编辑配置
sudo cp /etc/avahi/avahi-daemon.conf{,.bak}
sudo nano /etc/avahi/avahi-daemon.conf
```

修改或添加以下配置（`allow-interfaces` 改为你的网卡名）：

```ini
[server]
host-name=servers
domain-name=local
allow-interfaces=eth0
use-ipv4=yes
use-ipv6=yes
```

```bash
# 启用并重启服务
sudo systemctl enable --now avahi-daemon
sudo systemctl restart avahi-daemon
```

---

## Step 5: 配置 PM2（Node.js 进程管理）

### 5.1 配置服务 .env 文件

**Chatbot (`software_part/services/Chatbot/.env`)：**

```env
PORT=3333
HTTPS_ENABLED=true
HTTPS_KEY=certs/servers.local.key.pem
HTTPS_CERT=certs/servers.local.cert.pem
DEEPSEEK_API_KEY=your_deepseek_api_key_here
```

**Weather Service (`software_part/services/weather-service/.env`)：**

```env
PORT=3001
AMAP_API_KEY=your_amap_key_here
DEMO=0
```

### 5.2 安装依赖和 PM2

```bash
# 设置工程路径
PROJ=~/projects/2026_undergraduate_graduation_project

# 设置 npm 镜像（国内加速）
npm config set registry https://registry.npmmirror.com

# 安装 Chatbot 依赖
cd "$PROJ/software_part/services/Chatbot" && npm ci

# 全局安装 PM2
npm i -g pm2
hash -r
pm2 -v
```

### 5.3 启动服务

```bash
PROJ=~/projects/2026_undergraduate_graduation_project

# 启动 Chatbot
pm2 start "$PROJ/software_part/services/Chatbot/server.js" \
    --name chatbot \
    --cwd "$PROJ/software_part/services/Chatbot"

# 启动 Weather Service
pm2 start "$PROJ/software_part/services/weather-service/src/server.js" \
    --name weather-service \
    --cwd "$PROJ/software_part/services/weather-service"

# 保存配置
pm2 save

# 查看状态
pm2 list
```

**验证服务：**

```bash
curl -k https://127.0.0.1:3333/healthz ; echo
# 预期输出: {"ok":true}

curl http://127.0.0.1:3001/healthz ; echo
# 预期输出: ok
```

### 5.4 PM2 开机自启

```bash
pm2 startup
# 按照提示执行输出的命令
pm2 save
```

---

## Step 6: PSRAM 任务栈配置（可选）

如需调整 PSRAM 任务栈配置，可通过 `idf.py menuconfig` 或编辑 `software_part/sdkconfig` 修改以下选项：

```ini
CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY=y
CONFIG_APP_MUSIC_USE_PSRAM_STACK=y
CONFIG_APP_ENV_SHT40_USE_PSRAM_STACK=y
CONFIG_APP_WS2812_USE_PSRAM_STACK=y
```

**配置说明**：

- `CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY`：允许 FreeRTOS 任务栈使用 PSRAM（ESP-IDF 全局配置）
- `CONFIG_APP_MUSIC_USE_PSRAM_STACK`：music_task 使用 PSRAM 静态栈（6KB）
- `CONFIG_APP_ENV_SHT40_USE_PSRAM_STACK`：env_sht40 任务使用 PSRAM 栈（3KB）
- `CONFIG_APP_WS2812_USE_PSRAM_STACK`：ws2812 任务使用 PSRAM 栈（3KB）

**重要注意事项**：

- 使用 `xTaskCreatePinnedToCoreWithCaps()` 创建的任务（如 `env_sht40`、`splash_conn`、`wifi_stop`），自删除时必须调用 `vTaskDeleteWithCaps(NULL)` 而非 `vTaskDelete(NULL)`，否则会导致栈内存泄漏。
- OTA 任务**禁止**使用 PSRAM 栈（Flash 写入期间 cache 会被禁用）。

**验证方法**：

编译烧录后，monitor 日志中会出现类似输出（数值会随配置/运行时变化）：

```text
I (xxx) splash_ui: splash_conn: PSRAM stack (4KB)
I (xxx) wifi_ui: wifi_stop: PSRAM stack (3KB)
W (xxx) OTA_UPDATE: [OTA_DIAG] Before esp_https_ota_begin:
W (xxx) OTA_UPDATE:   Internal free=<bytes> largest=<bytes>
```

启用上述 PSRAM 栈配置后，可释放的"固定栈大小"是可核对的（按启用的任务而定），例如：

- music 6KB + ws2812 3KB + env_sht40 3KB + splash_conn 4KB + wifi_stop 3KB = **19KB**

更精确的内部堆/最大块大小请以 monitor 实际输出为准。

---

## Step 7: 配置 Caddy（Web 服务器 / 反向代理）

### 7.1 安装 Caddy

```bash
# 添加 Caddy 软件源
sudo tee /etc/apt/sources.list.d/caddy-stable.list >/dev/null <<'EOF'
deb [signed-by=/usr/share/keyrings/caddy-stable-archive-keyring.gpg] https://dl.cloudsmith.io/public/caddy/stable/deb/debian any-version main
EOF

# 导入 GPG 密钥
curl -fsSL 'https://dl.cloudsmith.io/public/caddy/stable/gpg.key' \
    | sudo gpg --dearmor --yes -o /usr/share/keyrings/caddy-stable-archive-keyring.gpg
sudo chmod 0644 /usr/share/keyrings/caddy-stable-archive-keyring.gpg

# 安装
sudo apt update
sudo apt install -y caddy
caddy version
```

### 7.2 配置 TLS 证书

```bash
sudo mkdir -p /etc/caddy/certs

# 将你的证书复制到此目录（自签证书生成方法请自行搜索或使用 mkcert）
# 建议统一用 servers.local 命名，避免与域名混淆
# sudo cp your-cert.pem /etc/caddy/certs/servers.local.cert.pem
# sudo cp your-key.pem  /etc/caddy/certs/servers.local.key.pem

sudo chown caddy:caddy /etc/caddy/certs/servers.local.cert.pem /etc/caddy/certs/servers.local.key.pem
sudo chmod 644 /etc/caddy/certs/servers.local.cert.pem
sudo chmod 600 /etc/caddy/certs/servers.local.key.pem
```

> 注意：ESP32 固件侧 HTTPS 默认信任 `server_root_cert_pem`（见 `software_part/components/02_middleware/audio_player/server_root_cert.c`）。
> 若你更换了 `servers.local` 的证书链/CA，需要同步更新固件内置证书并重新编译烧录，否则设备端 HTTPS 连接会校验失败。

### 7.3 Caddyfile 配置

创建 `/etc/caddy/Caddyfile`，内容如下（注意修改路径为你的实际工程目录）：

```caddyfile
{
    default_sni servers.local
}

https://servers.local {
    # ==================== OTA 固件分发 ====================
    handle /firmware {
        header Cache-Control "no-store"
        root * /home/YOUR_USER/projects/2026_undergraduate_graduation_project/software_part/services/OTA/fw/firmware
        rewrite * /undergraduate_project.bin
        file_server
    }

    # ==================== TLS 配置 ====================
    tls /etc/caddy/certs/servers.local.cert.pem /etc/caddy/certs/servers.local.key.pem {
        protocols tls1.2
        ciphers TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256
        curves secp256r1
        alpn http/1.1
    }

    # ==================== Chat UI ====================
    handle /chat {
        redir /chat/ 308
    }
    handle_path /chat/* {
        root * /home/YOUR_USER/projects/2026_undergraduate_graduation_project/software_part/services/Chatbot/public
        file_server
    }

    handle /favicon.ico {
        redir /chat/favicon.svg 308
    }

    # ==================== WebSocket 反向代理 ====================
    handle /ws* {
        reverse_proxy https://127.0.0.1:3333 {
            transport http {
                tls
                tls_server_name servers.local
                # Chatbot 若使用自签证书，需要在此处信任其 CA（本地可直接信任同一份 servers.local 证书）
                tls_trusted_ca_certs /etc/caddy/certs/servers.local.cert.pem
            }
        }
    }

    # ==================== 音乐服务 ====================
    root * /home/YOUR_USER/projects/2026_undergraduate_graduation_project/software_part/services/Music_Server
    header -Server

    # 音乐索引 JSON
    handle /index.json {
        header {
            Content-Type "application/json; charset=utf-8"
            Cache-Control "no-store"
            Access-Control-Allow-Origin "*"
            Access-Control-Expose-Headers "Content-Length, Accept-Ranges, Content-Range, ETag"
            X-Content-Type-Options "nosniff"
        }
        file_server
    }

    # ==================== 后台管理（Basic Auth） ====================
    @admin path /admin /admin/*
    basic_auth @admin {
        # 用户名: admin，密码需用 caddy hash-password 生成
        admin $2a$14$YOUR_HASHED_PASSWORD_HERE
    }
    @admin_api path /admin/api/*
    handle @admin_api {
        reverse_proxy 127.0.0.1:9000
    }
    handle @admin {
        header Cache-Control "no-store"
        file_server
    }

    # ==================== 访问日志 ====================
    log {
        output file /var/log/caddy/music-access.log {
            roll_size 20MiB
            roll_keep 10
        }
        format json
    }

    # ==================== 安全规则 ====================
    @secrets path *.pem *.key *.crt *.der *.p12 *.pfx
    respond @secrets 403

    @dotfiles path */.*
    respond @dotfiles 404

    @sensitive_ext path *.conf *.ini *.env *.md *.txt *.log *.yml *.yaml *.toml *.json *.sh *.ps1 *.bat *.cmd *.py *.rb *.php *.pl *.cgi *Caddyfile *Dockerfile *docker-compose*
    respond @sensitive_ext 403

    @non_audio {
        path_regexp file_with_ext ^.*/[^/]+\.[^/]+$
        not path_regexp audio (?i)^.*\.(wav|mp3|flac|ogg|opus|aac|m4a|mka|wma)$
    }
    respond @non_audio 403

    # ==================== 下载头 ====================
    @any_file path_regexp fname ^.*/([^/]+)$
    @dl_header {
        header X-Download-Name *
        query download=*
        not query download=1
        path_regexp fname ^.*/([^/]+)$
    }
    header @dl_header {
        defer
        Content-Disposition "attachment; filename=\"{http.regexp.fname.1}\"; filename*=UTF-8''{http.request.header.X-Download-Name}"
    }
    @dl_fallback {
        query download=1
        path_regexp fname ^.*/([^/]+)$
    }
    header @dl_fallback {
        defer
        Content-Disposition "attachment; filename=\"{http.regexp.fname.1}\"; filename*=UTF-8''{http.regexp.fname.1}"
    }

    # ==================== CORS 预检 ====================
    @options method OPTIONS
    header @options {
        Access-Control-Allow-Origin "*"
        Access-Control-Allow-Methods "GET, HEAD, OPTIONS"
        Access-Control-Allow-Headers "*"
        Access-Control-Max-Age "86400"
    }
    respond @options 204

    # ==================== 通用响应头 ====================
    @browse_page path_regexp dir ^.*/$
    header @browse_page Cache-Control "no-store"

    header {
        Access-Control-Allow-Origin "*"
        Access-Control-Expose-Headers "Content-Length, Accept-Ranges, Content-Range, ETag"
        Cache-Control "public, max-age=31536000, immutable"
        X-Content-Type-Options "nosniff"
    }

    encode zstd gzip
    file_server browse
}

# HTTP 重定向到 HTTPS
:80 {
    redir https://{host}{uri} 308
}
```

### 7.4 启动 Caddy

```bash
# 验证配置
sudo caddy validate --config /etc/caddy/Caddyfile

# 重载配置
sudo systemctl reload caddy

# 或重启服务
sudo systemctl restart caddy
sudo systemctl enable caddy
```

---

## Step 8: 配置 IoT 云平台 (Docker)

### 8.1 安装 Docker

```bash
sudo apt update
sudo apt install -y docker.io docker-compose-v2
sudo systemctl enable --now docker
sudo usermod -aG docker $USER
newgrp docker
docker compose version
```

### 8.2 配置国内镜像源（可选）

```bash
sudo mkdir -p /etc/docker
sudo tee /etc/docker/daemon.json >/dev/null <<'EOF'
{
  "registry-mirrors": ["https://docker.m.daocloud.io"]
}
EOF
sudo systemctl restart docker
```

### 8.3 启动 IoT 云平台

```bash
PROJ=~/projects/2026_undergraduate_graduation_project
cd $PROJ/software_part/services/IoT_cloud_platform/

# 拉取镜像
docker compose -f deploy/docker-compose.yml pull

# 启动服务（使用开发脚本）
./scripts/dev-up.sh

# 或直接使用 docker compose
# docker compose -f deploy/docker-compose.yml up -d
```

### 8.4 Docker 开机自启

Docker 服务默认开机自启。如需容器也自启，确保 `docker-compose.yml` 中配置了 `restart: unless-stopped`。

---

## 服务端口速查

| 端口  | 服务            | 说明           |
| ----- | --------------- | -------------- |
| 8088  | Nginx Gateway   | Web 前端入口   |
| 8080  | Spring Boot     | 后端 API       |
| 1883  | EMQX            | MQTT Broker    |
| 18083 | EMQX Dashboard  | MQTT 管理界面  |
| 3000  | Grafana         | 监控可视化     |
| 9091  | Prometheus      | 指标采集       |
| 3333  | Chatbot         | 语音对话服务   |
| 3001  | Weather Service | 天气代理       |
| 443   | Caddy           | HTTPS 主入口   |

---

## 常见问题

### Q: ESP-IDF 编译报错找不到组件？

确保已正确设置环境变量：

```bash
source ~/esp/esp-idf/export.sh
export ADF_PATH=~/esp/esp-adf
```

### Q: mDNS 无法解析 servers.local？

1. 检查 Avahi 服务状态：`systemctl status avahi-daemon`
2. 确认网卡名称正确配置在 `allow-interfaces`
3. 确保客户端和服务端在同一局域网

### Q: Docker 容器启动失败？

1. 检查日志：`docker compose logs -f`
2. 确认端口未被占用：`netstat -tlnp | grep <port>`
3. 检查 `.env` 文件配置是否正确
