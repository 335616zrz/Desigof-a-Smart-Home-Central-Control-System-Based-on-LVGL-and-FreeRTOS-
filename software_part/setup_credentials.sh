#!/bin/bash

# ============================================================================
# Credentials Setup Script
# ============================================================================
# 自动从 credentials.json 生成各服务的 .env 文件
# 使用方法: ./software_part/setup_credentials.sh [--force]
#
# 功能:
# 1. 检查 credentials.json 是否存在
# 2. 解析 JSON 并提取各服务凭证
# 3. 根据模板生成 .env 文件
# 4. 设置文件权限 (chmod 600)
# 5. 显示成功消息
#
# 参数:
#   --force    强制覆盖已存在的 .env 文件
# ============================================================================

set -e  # 遇到错误立即退出

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 关键文件路径
CREDENTIALS_FILE="$SCRIPT_DIR/credentials.json"
SERVICES_DIR="$SCRIPT_DIR/services"

# 强制覆盖标志
FORCE_OVERWRITE=false
if [[ "$1" == "--force" ]]; then
    FORCE_OVERWRITE=true
fi

# ============================================================================
# 工具函数
# ============================================================================

print_header() {
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

# 从 JSON 中提取值的函数 (使用 Python)
extract_json_value() {
    local json_file=$1
    local json_path=$2

    python3 -c "
import json, sys
try:
    with open('$json_file') as f:
        data = json.load(f)
    keys = '$json_path'.split('.')
    value = data
    for key in keys:
        value = value[key]
    print(value)
except Exception as e:
    print('', file=sys.stderr)
    sys.exit(1)
"
}

# 检查文件是否存在
check_file_exists() {
    local file=$1
    if [[ -f "$file" ]]; then
        if [[ "$FORCE_OVERWRITE" == true ]]; then
            print_warning "将覆盖已存在的文件: $file"
            return 0
        else
            print_warning "文件已存在: $file"
            read -p "是否覆盖? [y/N] " -n 1 -r
            echo
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                return 0
            else
                return 1
            fi
        fi
    fi
    return 0
}

# ============================================================================
# 主要逻辑
# ============================================================================

print_header "凭证配置安装脚本"

# 步骤 1: 检查 credentials.json 是否存在
print_info "步骤 1/4: 检查 credentials.json 文件..."

if [[ ! -f "$CREDENTIALS_FILE" ]]; then
    print_error "错误: credentials.json 文件不存在"
    print_info "文件路径: $CREDENTIALS_FILE"
    print_info ""
    print_info "请先创建 credentials.json 文件并填写真实凭证"
    print_info "参考: software_part/credentials.json.example (如果存在)"
    exit 1
fi

print_success "找到 credentials.json"

# 步骤 2: 检查 Python 是否可用 (用于解析 JSON)
print_info "步骤 2/4: 检查依赖..."

if ! command -v python3 &> /dev/null; then
    print_error "错误: 需要 Python 3 来解析 JSON 文件"
    print_info "请安装 Python 3: sudo apt install python3"
    exit 1
fi

print_success "Python 3 已安装"

# 步骤 3: 生成各服务的 .env 文件
print_info "步骤 3/4: 生成 .env 文件..."

# ============================================================================
# 3.1 生成 Chatbot/.env
# ============================================================================
CHATBOT_ENV="$SERVICES_DIR/Chatbot/.env"

print_info "  → 生成 Chatbot 服务配置..."

if check_file_exists "$CHATBOT_ENV"; then
    DEEPSEEK_KEY=$(extract_json_value "$CREDENTIALS_FILE" "chatbot_service.deepseek_api_key")
    COSYVOICE_KEY=$(extract_json_value "$CREDENTIALS_FILE" "chatbot_service.cosyvoice_api_key")
    CHATBOT_PORT=$(extract_json_value "$CREDENTIALS_FILE" "chatbot_service.port")
    HTTPS_ENABLED=$(extract_json_value "$CREDENTIALS_FILE" "chatbot_service.https_enabled")
    HTTPS_KEY=$(extract_json_value "$CREDENTIALS_FILE" "chatbot_service.https_key_path")
    HTTPS_CERT=$(extract_json_value "$CREDENTIALS_FILE" "chatbot_service.https_cert_path")

    cat > "$CHATBOT_ENV" << EOF
# DeepSeek API密钥 - 从 https://platform.deepseek.com/ 获取
DEEPSEEK_API_KEY=$DEEPSEEK_KEY

# 服务端口
PORT=$CHATBOT_PORT

# TTS配置（CosyVoice云端服务）
TTS_ENABLED=false
TTS_PROVIDER=local
TTS_FALLBACK_PROVIDER=none
COSYVOICE_API_KEY=$COSYVOICE_KEY
COSYVOICE_MODEL=cosyvoice-v3
COSYVOICE_VOICE=longhuhu_v3
COSYVOICE_FORMAT=pcm
COSYVOICE_SAMPLE_RATE=24000

# HTTPS/WSS设置
HTTPS_ENABLED=$HTTPS_ENABLED
HTTPS_KEY=$HTTPS_KEY
HTTPS_CERT=$HTTPS_CERT

# ASR流式识别设置
ASR_WORKER_SCRIPT=scripts/asr_worker_streaming.py
ASR_STREAMING_MODEL=paraformer-zh-streaming
ASR_STREAMING_MODEL_REVISION=v2.0.4
ASR_STREAMING_DEVICE=cpu

# 可选配置 (取消注释以启用)
# PORT=3000
# CORS_ALLOW_ORIGINS=https://example.com,https://another.example.com
# MAX_HISTORY_MESSAGES=10
# RATE_LIMIT_MAX=120
# REQUEST_TIMEOUT_MS=30000
# DEEPSEEK_MODEL=deepseek-chat
# TEMPERATURE=0.7
# MAX_TOKENS=1024
# SYSTEM_PROMPT=你是一名中文实时对话助手，请快速、准确、礼貌地回答用户问题。
EOF

    chmod 600 "$CHATBOT_ENV"
    print_success "已生成: services/Chatbot/.env"
else
    print_info "跳过 services/Chatbot/.env"
fi

# ============================================================================
# 3.2 生成 weather-service/.env
# ============================================================================
WEATHER_ENV="$SERVICES_DIR/weather-service/.env"

print_info "  → 生成 Weather Service 配置..."

if check_file_exists "$WEATHER_ENV"; then
    AMAP_KEY=$(extract_json_value "$CREDENTIALS_FILE" "weather_service.amap_api_key")
    WEATHER_PORT=$(extract_json_value "$CREDENTIALS_FILE" "weather_service.port")

    cat > "$WEATHER_ENV" << EOF
# 高德 Web 服务 API Key（不要提交到版本库）
AMAP_API_KEY=$AMAP_KEY
# 服务监听端口
PORT=$WEATHER_PORT
# 演示模式：1 为启用，将使用 mock 数据，不会向外网请求
DEMO=0
EOF

    chmod 600 "$WEATHER_ENV"
    print_success "已生成: services/weather-service/.env"
else
    print_info "跳过 services/weather-service/.env"
fi

# ============================================================================
# 3.3 生成 IoT_cloud_platform/deploy/.env
# ============================================================================
IOT_ENV="$SERVICES_DIR/IoT_cloud_platform/deploy/.env"

print_info "  → 生成 IoT Cloud Platform 配置..."

if check_file_exists "$IOT_ENV"; then
    MYSQL_ROOT_PASS=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.mysql.root_password")
    MYSQL_DB=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.mysql.database")
    MYSQL_USER=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.mysql.user")
    MYSQL_PASS=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.mysql.password")
    MYSQL_PORT=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.mysql.port")

    REDIS_PORT=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.redis.port")
    REDIS_PASS=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.redis.password")

    EMQX_DASH_USER=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.emqx.dashboard_username")
    EMQX_DASH_PASS=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.emqx.dashboard_password")
    EMQX_MQTT_USER=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.emqx.mqtt_app_username")
    EMQX_MQTT_PASS=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.emqx.mqtt_app_password")
    EMQX_MQTT_PORT=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.emqx.mqtt_port")
    EMQX_SSL_PORT=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.emqx.mqtt_ssl_port")
    EMQX_WS_PORT=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.emqx.websocket_port")
    EMQX_DASH_PORT=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.emqx.dashboard_port")

    JWT_SECRET=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.backend.jwt_secret")
    BACKEND_PORT=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.backend.port")

    APP_ADMIN_USERNAME=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.admin.username")
    APP_ADMIN_PASSWORD=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.admin.password")
    APP_ADMIN_EMAIL=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.admin.email")

    GRAFANA_USER=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.grafana.admin_user")
    GRAFANA_PASS=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.grafana.admin_password")
    GRAFANA_PORT=$(extract_json_value "$CREDENTIALS_FILE" "iot_cloud_platform.grafana.port")

    cat > "$IOT_ENV" << EOF
# ========================================
# IoT Cloud Platform 环境变量配置
# ========================================
# 由 setup_credentials.sh 自动生成
# 生成时间: $(date '+%Y-%m-%d %H:%M:%S')

# ========================================
# MySQL 数据库配置
# ========================================
MYSQL_ROOT_PASSWORD=$MYSQL_ROOT_PASS
MYSQL_DATABASE=$MYSQL_DB
MYSQL_USER=$MYSQL_USER
MYSQL_PASSWORD=$MYSQL_PASS
MYSQL_PORT=$MYSQL_PORT

# ========================================
# Redis 配置
# ========================================
REDIS_PORT=$REDIS_PORT
EOF

    # 如果 Redis 密码不为空，则添加密码配置
    if [[ -n "$REDIS_PASS" ]]; then
        echo "REDIS_PASSWORD=$REDIS_PASS" >> "$IOT_ENV"
    else
        echo "# REDIS_PASSWORD=  # 未设置密码" >> "$IOT_ENV"
    fi

    cat >> "$IOT_ENV" << EOF

# ========================================
# EMQX MQTT Broker 配置
# ========================================
EMQX_DASHBOARD_USERNAME=$EMQX_DASH_USER
EMQX_DASHBOARD_PASSWORD=$EMQX_DASH_PASS
EMQX_MQTT_USERNAME=$EMQX_MQTT_USER
EMQX_MQTT_PASSWORD=$EMQX_MQTT_PASS
EMQX_MQTT_PORT=$EMQX_MQTT_PORT
EMQX_MQTT_SSL_PORT=$EMQX_SSL_PORT
EMQX_WEBSOCKET_PORT=$EMQX_WS_PORT
EMQX_DASHBOARD_PORT=$EMQX_DASH_PORT

# ========================================
# Spring Boot Backend 配置
# ========================================
BACKEND_PORT=$BACKEND_PORT
JWT_SECRET=$JWT_SECRET
SPRING_PROFILES_ACTIVE=docker

# ========================================
# Backend 管理员初始化（必填）
# ========================================
APP_ADMIN_USERNAME=$APP_ADMIN_USERNAME
APP_ADMIN_PASSWORD=$APP_ADMIN_PASSWORD
APP_ADMIN_EMAIL=$APP_ADMIN_EMAIL

# ========================================
# Grafana 配置
# ========================================
GRAFANA_ADMIN_USER=$GRAFANA_USER
GRAFANA_ADMIN_PASSWORD=$GRAFANA_PASS
GRAFANA_PORT=$GRAFANA_PORT

# ========================================
# Frontend Gateway 配置
# ========================================
GATEWAY_PORT=8088
EOF

    chmod 600 "$IOT_ENV"
    print_success "已生成: services/IoT_cloud_platform/deploy/.env"
else
    print_info "跳过 services/IoT_cloud_platform/deploy/.env"
fi

# 步骤 4: 验证文件权限
print_info "步骤 4/4: 验证文件权限..."

for env_file in "$CHATBOT_ENV" "$WEATHER_ENV" "$IOT_ENV"; do
    if [[ -f "$env_file" ]]; then
        perms=$(stat -c "%a" "$env_file" 2>/dev/null || stat -f "%A" "$env_file" 2>/dev/null)
        if [[ "$perms" == "600" ]]; then
            print_success "$(basename $(dirname $env_file))/.env 权限正确 (600)"
        else
            print_warning "$(basename $(dirname $env_file))/.env 权限: $perms (建议: 600)"
        fi
    fi
done

# ============================================================================
# 完成
# ============================================================================

print_header "配置完成"

echo ""
print_success "所有 .env 文件已成功生成！"
echo ""
print_info "下一步操作:"
echo "  1. 验证生成的 .env 文件内容是否正确"
echo "  2. 启动各服务:"
echo "     - Chatbot:            cd services/Chatbot && npm run dev"
echo "     - Weather Service:    cd services/weather-service && npm start"
echo "     - IoT Cloud Platform: cd services/IoT_cloud_platform/deploy && docker compose up -d"
echo ""
print_warning "安全提醒:"
echo "  ✓ 这些 .env 文件已添加到 .gitignore，不会被提交到 Git"
echo "  ✓ 文件权限已设置为 600 (仅所有者可读写)"
echo "  ✗ 永远不要手动将 .env 文件添加到 Git"
echo "  ✗ 定期更换 API keys 和密码"
echo ""
