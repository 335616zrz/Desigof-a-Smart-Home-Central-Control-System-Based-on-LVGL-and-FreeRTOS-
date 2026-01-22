# 安全扫描报告（可复现）

> 本文档记录项目的安全扫描方法、结果与敏感文件管理规范。
> 目标：任何人 clone 本仓库后都能复现本报告的结论。

---

## 扫描结果概览

**状态：** ✅ 在本报告定义的扫描范围与方法下，未发现明显恶意代码特征

> 注：这不等同于"无安全风险/无漏洞"。代码审查发现的已知风险与改进建议见下方"已知风险/待改进"。

**最后更新：** 2026-01-22

---

## 扫描范围与方法（可复现）

### 扫描范围

- 扫描对象：本仓库 *git 跟踪的第一方源码/脚本*（不扫描本地 `.env` / 私钥等敏感文件，因为这些默认不应提交）。
- 第一方目录（纳入"恶意代码检查"结论）：
  - `software_part/main/`（固件主程序）
  - `software_part/components/`（固件组件，不含 `software_part/managed_components/`）
  - `software_part/tools/`（固件侧工具脚本）
  - `software_part/services/Chatbot/src/`、`software_part/services/Chatbot/scripts/`
  - `software_part/services/weather-service/src/`
  - `software_part/services/IoT_cloud_platform/backend/src/`、`software_part/services/IoT_cloud_platform/frontend/src/`
  - `software_part/services/WeChat_Mini_Program/pages/`、`software_part/services/WeChat_Mini_Program/components/`
  - `tools/`（脚本 / Git Hook）
- 第三方/生成物（默认不计入结论，但会在下方"第三方命中"中说明）：
  - `software_part/managed_components/`（ESP 组件管理器下载的第三方源码）
  - `software_part/services/**/node_modules/`、`software_part/services/**/.venv*/`
  - `software_part/tools/venv/`（本地 Python 虚拟环境）
  - `software_part/services/WeChat_Mini_Program/unpackage/`、`software_part/**/dist/`、`software_part/build/`

### 复现命令

在仓库根目录执行；若命令无输出，表示未命中。
提示：`git grep` 默认只扫描 git 跟踪文件；如需把未 `git add` 的新增文件也纳入扫描，可先 `git add -N <path>`（不真正暂存内容）或改用 `rg -n -S` 对工作区扫描。

```bash
# 0) 敏感文件/误提交检查（同时适合作为 pre-commit hook）
./tools/check_security.sh

# 1) 定义"第一方代码"扫描路径（可按需增减）
SCAN_PATHS=(
  software_part/main
  software_part/components
  software_part/tools
  software_part/services/Chatbot/src
  software_part/services/Chatbot/scripts
  software_part/services/weather-service/src
  software_part/services/IoT_cloud_platform/backend/src
  software_part/services/IoT_cloud_platform/frontend/src
  software_part/services/WeChat_Mini_Program/pages
  software_part/services/WeChat_Mini_Program/components
  tools
)

# 2) 反弹 Shell / 后门关键字（常见：nc -e、/dev/tcp、mkfifo 管道）
git grep -n -E '(nc -e|/dev/tcp|mkfifo)' -- "${SCAN_PATHS[@]}"

# 3) 加密货币挖矿关键字
git grep -n -E '(coinhive|stratum|xmrig|cryptonight)' -- "${SCAN_PATHS[@]}"

# 4) 原始套接字/嗅探关键字
git grep -n -E '(SOCK_RAW|libpcap|pcap_|promiscuous|AF_PACKET)' -- "${SCAN_PATHS[@]}"

# 5) 命令执行入口（第一方）：eval / child_process / Runtime.exec / ProcessBuilder / popen / system
git grep -n -E '(eval[[:space:]]*\(|child_process\.(exec|spawn|execFile|fork)|Runtime\.getRuntime\(\)\.exec|ProcessBuilder|popen[[:space:]]*\(|system[[:space:]]*\()' -- "${SCAN_PATHS[@]}"

# 6) 可疑二进制文件（git 跟踪文件中）
git ls-files | rg -n '\.(so|dll|exe|dylib)$'
```

### 本次扫描命中（证据）

- 业务需要的进程调用（非隐蔽执行）：

```bash
git grep -n 'ProcessBuilder' -- software_part/services/IoT_cloud_platform/backend/src
```

- 第三方依赖/工具链中的"易误报"关键字（属于正常）：

```bash
# PNG 压缩库策略枚举（BRUTE_FORCE）
git grep -n 'BRUTE_FORCE' -- software_part/managed_components/lvgl__lvgl/src/libs/lodepng

# nghttp2/nghttpx 等工具中的 setuid/setgid（用于降权运行）
git grep -n -E 'set[ug]id' -- software_part/managed_components/espressif__nghttp/nghttp2

# mruby-socket 中的 SOCK_RAW 常量定义（未代表本项目启用原始套接字）
git grep -n 'SOCK_RAW' -- software_part/managed_components/espressif__nghttp/nghttp2/third-party/mruby/mrbgems/mruby-socket
```

---

## 详细扫描结果

### 恶意代码检查（基于关键字/模式）

| 检查项                 | 结果                         | 说明 / 证据                                                                                                                                                                                                                           |
| ---------------------- | ---------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 反弹 Shell / 后门      | ✅ 未发现（第一方范围内）    | 复现：见"复现命令"第 2 条                                                                                                                                                                                                             |
| 加密货币挖矿           | ✅ 未发现（第一方范围内）    | 复现：见"复现命令"第 3 条                                                                                                                                                                                                             |
| 暴力破解 / DDoS        | ✅ 未发现（第一方范围内）    | 第三方 LVGL 的 lodepng 中存在 `LFS_BRUTE_FORCE`（PNG 压缩策略枚举，见上方"第三方命中"）                                                                                                                                               |
| 命令执行               | ⚠️ 存在（业务功能）          | 后端使用 `ProcessBuilder` 调用 `ffprobe`/`ffmpeg` 做音频时长/转码：`software_part/services/IoT_cloud_platform/backend/src/main/java/iot/music/MusicFileService.java:292`、`software_part/services/IoT_cloud_platform/backend/src/main/java/iot/music/TranscodeService.java:67` |
| 数据窃取 / 键盘记录    | ✅ 未发现（第一方范围内）    | 复现（可选）：`git grep -n -E '(keylog|GetAsyncKeyState|SetWindowsHookEx|SSLKEYLOGFILE|/dev/input)' -- "${SCAN_PATHS[@]}"`                                                                                                            |
| 原始套接字 / 嗅探      | ✅ 未发现（第一方范围内）    | 第三方依赖中可能出现 `SOCK_RAW` 常量定义（见上方"第三方命中"），不代表本项目启用抓包/嗅探                                                                                                                                             |
| 可疑二进制文件         | ✅ 未发现（git 跟踪文件）    | 复现：见"复现命令"第 6 条                                                                                                                                                                                                             |
| 权限提升代码           | ✅ 未发现（第一方范围内）    | 第三方 nghttp2/nghttpx 包含 `setuid/setgid` 用于降权运行（见上方"第三方命中"）                                                                                                                                                        |

### 外部网络连接分析（基于源码）

| URL / 域名                | 端口 | 用途                             | 证据（命中位置）                                                                                                               |
| ------------------------- | ---- | -------------------------------- | ------------------------------------------------------------------------------------------------------------------------------ |
| `servers.local`           | 443  | 局域网 HTTPS（固件、音乐、反代） | 固件/配置默认值：`software_part/sdkconfig.defaults`；部署：`docs/DEPLOYMENT.md`                                                |
| `api.deepseek.com`        | 443  | DeepSeek 对话 API                | `software_part/services/Chatbot/src/realtime/RealtimeSession.js`                                                               |
| `restapi.amap.com`        | 443  | 高德天气 API                     | `software_part/services/weather-service/src/server.js`                                                                         |
| `dashscope.aliyuncs.com`  | 443  | CosyVoice TTS（WebSocket / HTTP）| `software_part/services/Chatbot/src/realtime/RealtimeSession.js`、`software_part/services/Chatbot/scripts/`                   |

> 说明：上述外联仅在对应功能启用且配置了 API Key 时发生；本地开发环境也可能访问 `127.0.0.1` / `0.0.0.0`。

### 数据存储分析

| 存储位置       | 内容                                      | 证据（命中位置）                                                     |
| -------------- | ----------------------------------------- | -------------------------------------------------------------------- |
| `localStorage` | 主题、API Base、MQTT WS、JWT/refresh token| `software_part/services/IoT_cloud_platform/frontend/src/stores/*.ts` |
| `NVS (ESP32)`  | Wi-Fi 配置等                              | ESP-IDF 标准行为（Wi-Fi STA 配网）                                   |
| `SD Card`      | 音乐文件、日志                            | `software_part/services/Music_Server/`（文件）与固件播放器逻辑       |

### 第三方依赖来源（可核对）

| 依赖                                   | 来源/配置依据                                                      | 备注                                       |
| -------------------------------------- | ------------------------------------------------------------------ | ------------------------------------------ |
| ESP-IDF / 组件依赖（含 LVGL、ESP-SR 等）| `software_part/dependencies.lock` / `software_part/idf_component.yml` | 由 ESP Component Manager 管理（版本/哈希可追溯） |
| IoT 后端（Java）                       | `software_part/services/IoT_cloud_platform/backend/pom.xml`        | Maven 依赖                                 |
| IoT 前端（Vue）                        | `software_part/services/IoT_cloud_platform/frontend/package.json`  | npm 依赖                                   |
| Chatbot / weather-service（Node）      | 各自 `package.json` / lock                                         | npm 依赖                                   |
| Chatbot（Python）                      | `software_part/services/Chatbot/requirements.txt`                  | PyPI 依赖（运行环境自建 venv）             |

### 新增组件安全检查（2026-01-22）

| 组件             | 检查项            | 结果                                                                                                                                                  |
| ---------------- | ----------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------- |
| `asr_client`     | WebSocket TLS 验证| TLS 模式下默认要求提供 `cert_pem`；仅在编译选项开启 `*_TLS_INSECURE` 时跳过校验（见 `software_part/components/02_middleware/asr_client/asr_client.c`）|
| `mem_utils`      | PSRAM 分配        | 使用 `heap_caps_malloc/heap_caps_free` 等接口；包含简单诊断与 fallback（见 `software_part/components/common/mem_utils/include/mem_utils.h`）          |
| `voice_commands` | 命令注入          | 本地命令词表固定；不包含 shell/动态执行逻辑（见 `software_part/components/01_application/voice_commands/voice_commands.c`）                           |

### 已知风险/待改进（来自源码审查）

以下问题不属于"恶意代码"，但可能造成安全或稳定性风险；均可通过命令定位到代码位置：

1. **设备端 WebSocket JSON 重组未设置上限（潜在 DoS）**
   - 现象：按对端 `payload_len` 直接 `malloc(total + 1)`，若对端异常/恶意发送超大帧，可能触发内存耗尽/重启。
   - 证据：`software_part/components/01_application/my_ui/custom/chatbot_client.c:248` ~ `software_part/components/01_application/my_ui/custom/chatbot_client.c:266`（`payload_len` 重组逻辑）。
   - 复现：`git grep -n 'payload_len' -- software_part/components/01_application/my_ui/custom/chatbot_client.c`

2. **TTS chunk base64 解码未限制输入长度（潜在 DoS）**
   - 现象：`outcap` 由 base64 字符串长度推导并直接 `malloc(outcap)`；缺少设备侧上限，可能被异常/恶意服务端触发大分配。
   - 证据：`software_part/components/01_application/my_ui/custom/chatbot_client.c:199` ~ `software_part/components/01_application/my_ui/custom/chatbot_client.c:216`（`mbedtls_base64_decode`）。
   - 复现：`git grep -n 'mbedtls_base64_decode' -- software_part/components/01_application/my_ui/custom/chatbot_client.c`

3. **TLS 主机名校验跳过/可被关闭（降低抗 MITM 能力）**
   - Chatbot：`MY_UI_CHATBOT_TLS_SKIP_CN` 默认开启（Kconfig），会设置 `skip_cert_common_name_check=true`；另有 `MY_UI_CHATBOT_TLS_INSECURE`（完全不校验，开发期）。
   - OTA / Music：`sdkconfig.defaults` 中默认开启 `*_SKIP_CN_CHECK`（仅跳过 CN/SAN 校验，仍做证书链校验）。
   - 复现：
     - `git grep -n 'MY_UI_CHATBOT_TLS_SKIP_CN\\|MY_UI_CHATBOT_TLS_INSECURE' -- software_part/components/01_application/my_ui/Kconfig.projbuild`
     - `git grep -n 'SKIP_CN_CHECK' -- software_part/sdkconfig.defaults`

### 内存与运行时安全（说明）

- 本项目的 `sdkconfig.defaults` 默认开启了轻量堆投毒：`CONFIG_HEAP_POISONING_LIGHT=y`（见 `software_part/sdkconfig.defaults`）。
- FreeRTOS 栈溢出检查/Watchpoint 属于可选项，是否开启以实际 `sdkconfig` 为准（仓库只提交 `sdkconfig.defaults` 作为基线配置）。

---

## 敏感文件清单

以下文件包含敏感信息，**绝对不可**提交到 Git：

| 文件               | 说明                               |
| ------------------ | ---------------------------------- |
| `credentials.json` | 主凭据文件（API Keys、数据库密码等）|
| `*.env`            | 各服务环境变量文件                 |
| `sdkconfig`        | ESP-IDF 配置（可能包含 Wi-Fi 密码）|
| `certs/*.key.pem`  | TLS 私钥                           |
| `*.p12` / `*.pfx`  | 证书文件                           |

---

## 脱敏建议

### 1. Wi-Fi 配置

`sdkconfig.defaults` 中的 Wi-Fi SSID/密码应使用占位符：

```ini
CONFIG_NET_WIFI_SSID="YOUR_WIFI_SSID"
CONFIG_NET_WIFI_PASSWORD="YOUR_WIFI_PASSWORD"
```

本地通过 `sdkconfig` 或 `idf.py menuconfig` 覆盖。

### 2. API 密钥

使用 `credentials.json` 集中管理，通过 `setup_credentials.sh` 分发到各服务的 `.env` 文件。

### 3. 数据库密码

生产环境使用强密码（16位以上随机字符串），通过环境变量注入。

### 4. 防止误提交

运行 Git 安全检查钩子，在 commit 前自动扫描敏感文件：

```bash
./tools/install_hooks.sh
```

---

## 安全检查工具

项目提供自动化安全检查工具：

| 工具                 | 位置                       | 用途                                              |
| -------------------- | -------------------------- | ------------------------------------------------- |
| `check_security.sh`  | `tools/check_security.sh`  | 手动扫描敏感文件（包括密钥、证书、密码等）        |
| `install_hooks.sh`   | `tools/install_hooks.sh`   | 安装 Git pre-commit 钩子（每次 commit 前自动检查）|

**用法**：

```bash
# 手动检查
./tools/check_security.sh

# 安装自动检查钩子（推荐）
./tools/install_hooks.sh
```
