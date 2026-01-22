#!/bin/bash

# ============================================================================
# Git Security Check Script
# ============================================================================
# 防止敏感文件被意外提交到 Git
# 用法:
#   1. 手动运行: ./tools/check_security.sh
#   2. 作为 pre-commit hook: 复制到 .git/hooks/pre-commit
#
# 安装为 Git Hook:
#   cp tools/check_security.sh .git/hooks/pre-commit
#   chmod +x .git/hooks/pre-commit
# ============================================================================

set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 是否作为 Git Hook 运行
IS_GIT_HOOK=false
if [[ "$(basename "$SCRIPT_DIR")" == "hooks" ]]; then
    IS_GIT_HOOK=true
    PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
fi

# ============================================================================
# 工具函数
# ============================================================================

print_header() {
    echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${GREEN}  $1${NC}"
    echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

print_error() {
    echo -e "${RED}✗ 错误: $1${NC}" >&2
}

print_warning() {
    echo -e "${YELLOW}⚠ 警告: $1${NC}" >&2
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

# ============================================================================
# 禁止提交的文件列表
# ============================================================================

# 敏感凭证文件（绝对路径模式）
FORBIDDEN_FILES=(
    "software_part/credentials.json"
    "software_part/services/Chatbot/.env"
    "software_part/services/weather-service/.env"
    "software_part/services/IoT_cloud_platform/deploy/.env"
    "software_part/sdkconfig"
)

# 文件扩展名模式（全局匹配）
FORBIDDEN_PATTERNS=(
    "*.pem"
    "*.key"
    "*.crt"
    "*.p12"
    "*.pfx"
    "*.jks"
    "*.keystore"
)

# 允许的例外（即使匹配上述模式也可以提交）
ALLOWED_EXCEPTIONS=(
    "*.example"
    "*.env.example"
    "*.pem.example"
    ".env.example"
)

# ============================================================================
# 检测敏感文件
# ============================================================================

check_sensitive_files() {
    local files_to_check=()
    local found_sensitive=false

    # 获取待提交的文件列表
    if [[ "$IS_GIT_HOOK" == true ]]; then
        # 作为 Git Hook 运行：检查暂存区文件
        mapfile -t files_to_check < <(git diff --cached --name-only --diff-filter=ACM)
    else
        # 手动运行：检查所有已修改和未跟踪的文件
        mapfile -t files_to_check < <(git diff --name-only --diff-filter=ACM)
        mapfile -t -O "${#files_to_check[@]}" files_to_check < <(git ls-files --others --exclude-standard)
    fi

    if [[ ${#files_to_check[@]} -eq 0 ]]; then
        print_success "没有待检查的文件"
        return 0
    fi

    echo "检查 ${#files_to_check[@]} 个文件..."
    echo ""

    # 检查每个文件
    for file in "${files_to_check[@]}"; do
        local is_forbidden=false
        local reason=""

        # 检查文件是否在允许列表中
        local is_allowed=false
        for exception in "${ALLOWED_EXCEPTIONS[@]}"; do
            if [[ "$file" == $exception ]] || [[ "$(basename "$file")" == $exception ]]; then
                is_allowed=true
                break
            fi
        done

        if [[ "$is_allowed" == true ]]; then
            continue
        fi

        # 检查绝对路径匹配
        for forbidden in "${FORBIDDEN_FILES[@]}"; do
            if [[ "$file" == "$forbidden" ]]; then
                is_forbidden=true
                reason="敏感凭证文件"
                break
            fi
        done

        # 检查文件扩展名模式
        if [[ "$is_forbidden" == false ]]; then
            for pattern in "${FORBIDDEN_PATTERNS[@]}"; do
                if [[ "$file" == $pattern ]]; then
                    is_forbidden=true
                    reason="敏感文件类型 ($pattern)"
                    break
                fi
            done
        fi

        # 检查 .env 文件（但允许 .env.example）
        if [[ "$is_forbidden" == false ]] && [[ "$file" =~ \.env$ ]] && [[ ! "$file" =~ \.env\.example$ ]]; then
            is_forbidden=true
            reason="环境变量配置文件"
        fi

        # 如果检测到敏感文件
        if [[ "$is_forbidden" == true ]]; then
            found_sensitive=true
            print_error "检测到敏感文件: $file"
            echo "         原因: $reason"
            echo ""
        fi
    done

    if [[ "$found_sensitive" == true ]]; then
        return 1
    else
        return 0
    fi
}

# ============================================================================
# 检测硬编码的密钥（可选，需要 grep）
# ============================================================================

check_hardcoded_secrets() {
    local files_to_check=()

    # 获取待提交的源代码文件
    if [[ "$IS_GIT_HOOK" == true ]]; then
        mapfile -t files_to_check < <(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(js|ts|py|java|sh|yml|yaml|json|md)$' || true)
    else
        mapfile -t files_to_check < <(git diff --name-only --diff-filter=ACM | grep -E '\.(js|ts|py|java|sh|yml|yaml|json|md)$' || true)
    fi

    if [[ ${#files_to_check[@]} -eq 0 ]]; then
        return 0
    fi

    local found_secrets=false

    # 常见的 API key 模式
    local patterns=(
        "sk-[a-zA-Z0-9]{32,}"          # DeepSeek/OpenAI style
        "api[_-]?key['\"]?\s*[:=]\s*['\"][^'\"]{16,}"  # API key assignments
        "password['\"]?\s*[:=]\s*['\"][^'\"]{8,}"      # Password assignments (excluding placeholders)
        "secret['\"]?\s*[:=]\s*['\"][^'\"]{16,}"       # Secret assignments
        "token['\"]?\s*[:=]\s*['\"][^'\"]{16,}"        # Token assignments
    )

    for file in "${files_to_check[@]}"; do
        if [[ ! -f "$file" ]]; then
            continue
        fi

        for pattern in "${patterns[@]}"; do
            if grep -qiE "$pattern" "$file" 2>/dev/null; then
                # 排除已知的安全示例
                if grep -qE "(YOUR_|CHANGE_ME|PLACEHOLDER|example|your_api_key_here)" "$file" 2>/dev/null; then
                    continue
                fi

                print_warning "文件 $file 可能包含硬编码的密钥"
                found_secrets=true
            fi
        done
    done

    if [[ "$found_secrets" == true ]]; then
        echo ""
        print_warning "请确认上述文件中的密钥是否为占位符或示例"
    fi

    return 0  # 仅警告，不阻止提交
}

# ============================================================================
# 主程序
# ============================================================================

print_header "Git 安全检查"

echo ""

# 执行敏感文件检查
if ! check_sensitive_files; then
    echo ""
    print_error "发现敏感文件！"
    echo ""
    echo "如需移除这些文件，请执行:"
    echo "  git reset HEAD <文件名>"
    echo "  git restore --staged <文件名>"
    echo ""
    echo "如需将文件添加到 .gitignore:"
    echo "  echo '<文件名>' >> .gitignore"
    echo ""

    if [[ "$IS_GIT_HOOK" == true ]]; then
        print_error "提交已被阻止"
        exit 1
    else
        exit 1
    fi
fi

# 执行硬编码密钥检查（仅警告）
check_hardcoded_secrets

echo ""
print_success "安全检查通过！"
echo ""

exit 0
