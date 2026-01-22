#!/bin/bash

# ============================================================================
# Pre-commit Hook 安装脚本
# ============================================================================
# 自动安装 Git pre-commit hook，防止敏感文件被提交
# ============================================================================

set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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

# ============================================================================
# 主程序
# ============================================================================

print_header "Git Pre-commit Hook 安装"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
SECURITY_SCRIPT="$SCRIPT_DIR/check_security.sh"
HOOK_DIR="$PROJECT_ROOT/.git/hooks"
HOOK_FILE="$HOOK_DIR/pre-commit"

echo ""

# 检查是否在 Git 仓库中
if [[ ! -d "$PROJECT_ROOT/.git" ]]; then
    print_error "错误: 未找到 .git 目录"
    print_info "请确保在 Git 仓库根目录运行此脚本"
    exit 1
fi

print_success "找到 Git 仓库"

# 检查安全脚本是否存在
if [[ ! -f "$SECURITY_SCRIPT" ]]; then
    print_error "错误: 未找到安全检查脚本"
    print_info "预期路径: $SECURITY_SCRIPT"
    exit 1
fi

print_success "找到安全检查脚本"

# 创建 hooks 目录（如果不存在）
if [[ ! -d "$HOOK_DIR" ]]; then
    mkdir -p "$HOOK_DIR"
    print_success "创建 hooks 目录"
fi

# 检查是否已存在 pre-commit hook
if [[ -f "$HOOK_FILE" ]]; then
    print_warning "检测到已存在的 pre-commit hook"

    # 检查是否是我们的脚本
    if grep -q "Git Security Check Script" "$HOOK_FILE" 2>/dev/null; then
        print_info "现有 hook 是安全检查脚本，将覆盖"
    else
        # 备份现有 hook
        BACKUP_FILE="$HOOK_FILE.backup.$(date +%Y%m%d_%H%M%S)"
        cp "$HOOK_FILE" "$BACKUP_FILE"
        print_warning "已备份现有 hook 到: $BACKUP_FILE"
    fi
fi

# 复制脚本到 hooks 目录
cp "$SECURITY_SCRIPT" "$HOOK_FILE"
chmod +x "$HOOK_FILE"

print_success "已安装 pre-commit hook"

echo ""
print_header "安装完成"
echo ""

print_success "Git pre-commit hook 已成功安装！"
echo ""
print_info "功能:"
echo "  • 自动检测敏感文件（.env, credentials.json, *.pem, *.key 等）"
echo "  • 在提交前阻止敏感文件被提交到 Git"
echo "  • 检测代码中可能硬编码的密钥（仅警告）"
echo ""
print_info "测试 hook:"
echo "  1. 尝试添加一个 .env 文件: touch test.env && git add test.env"
echo "  2. 尝试提交: git commit -m \"test\""
echo "  3. Hook 应该会阻止提交并显示错误信息"
echo ""
print_info "卸载 hook:"
echo "  rm .git/hooks/pre-commit"
echo ""
print_warning "注意:"
echo "  • Git hooks 不会被提交到仓库（.git/hooks 在 .gitignore 中）"
echo "  • 每个开发者需要在本地单独安装"
echo "  • 可以使用 git commit --no-verify 绕过 hook（不推荐）"
echo ""
