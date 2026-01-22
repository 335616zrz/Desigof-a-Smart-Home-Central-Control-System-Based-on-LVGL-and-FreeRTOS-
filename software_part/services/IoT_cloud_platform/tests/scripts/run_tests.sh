#!/bin/bash
# IoT Cloud Platform - 统一测试脚本
# 提供快捷的测试命令入口

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 显示使用说明
show_usage() {
    echo -e "${BLUE}IoT Cloud Platform - 统一测试脚本${NC}"
    echo ""
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  api            运行API测试"
    echo "  mqtt           运行MQTT测试"
    echo "  integration    运行集成测试"
    echo "  e2e            运行E2E测试 (需要浏览器)"
    echo "  all            运行所有测试"
    echo "  clean          清理测试缓存和临时文件"
    echo "  help           显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  $0 api"
    echo "  $0 all"
    echo ""
}

# 检查Python环境
check_python() {
    if ! command -v python3 &> /dev/null; then
        echo -e "${RED}错误: 未找到 python3${NC}"
        exit 1
    fi
}

# 运行API测试
run_api_tests() {
    echo -e "${BLUE}运行 API 测试...${NC}"
    python3 tests/api/test_api_endpoints.py
}

# 运行MQTT测试
run_mqtt_tests() {
    echo -e "${BLUE}运行 MQTT 测试...${NC}"
    python3 tests/integration/test_mqtt_client.py
}

# 运行集成测试
run_integration_tests() {
    echo -e "${BLUE}运行集成测试...${NC}"
    python3 tests/integration/test_comprehensive.py
}

# 运行所有测试
run_all_tests() {
    echo -e "${BLUE}运行所有测试...${NC}"
    echo ""

    echo -e "${YELLOW}[1/3] API 测试${NC}"
    run_api_tests

    echo ""
    echo -e "${YELLOW}[2/3] MQTT 测试${NC}"
    run_mqtt_tests

    echo ""
    echo -e "${YELLOW}[3/3] 集成测试${NC}"
    run_integration_tests

    echo ""
    echo -e "${GREEN}✓ 所有测试完成${NC}"
}

# 清理测试文件
clean_tests() {
    echo -e "${BLUE}清理测试缓存和临时文件...${NC}"

    # 清理Python缓存
    find tests -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
    find tests -type f -name "*.pyc" -delete 2>/dev/null || true
    find tests -type f -name "*.pyo" -delete 2>/dev/null || true

    # 清理截图文件
    rm -f *.png 2>/dev/null || true

    # 清理临时文件
    rm -f /tmp/token.json 2>/dev/null || true

    echo -e "${GREEN}✓ 清理完成${NC}"
}

# 主函数
main() {
    check_python

    case "${1:-help}" in
        api)
            run_api_tests
            ;;
        mqtt)
            run_mqtt_tests
            ;;
        integration)
            run_integration_tests
            ;;
        all)
            run_all_tests
            ;;
        clean)
            clean_tests
            ;;
        help|--help|-h)
            show_usage
            ;;
        *)
            echo -e "${RED}错误: 未知选项 '$1'${NC}"
            echo ""
            show_usage
            exit 1
            ;;
    esac
}

# 执行主函数
main "$@"
