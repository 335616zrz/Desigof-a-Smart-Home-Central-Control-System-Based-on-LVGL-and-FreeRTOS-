#!/usr/bin/env python3
"""
综合集成测试模块 - 测试前端、后端、MQTT、监控系统
"""

import os
import requests
import json
import time
import socket
import sys
from datetime import datetime


class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    END = '\033[0m'


BASE_URL = "http://localhost:8088"
API_URL = "http://localhost:8080/api"
ADMIN_USERNAME = os.getenv("APP_ADMIN_USERNAME", "admin")
ADMIN_PASSWORD = os.getenv("APP_ADMIN_PASSWORD", "")


def print_header(text):
    print(f"\n{Colors.BLUE}{'='*60}")
    print(f"{text}")
    print(f"{'='*60}{Colors.END}\n")


def print_test(name, status, details=""):
    icon = f"{Colors.GREEN}✅" if status else f"{Colors.RED}❌"
    print(f"{icon} {name}{Colors.END}")
    if details:
        print(f"   {details}")


def test_frontend_accessibility():
    """测试前端可访问性"""
    print_header("前端可访问性测试")

    try:
        response = requests.get(BASE_URL, timeout=5)
        if response.status_code == 200:
            print_test("前端页面加载", True, f"状态码: {response.status_code}")
            return True
        else:
            print_test("前端页面加载", False, f"状态码: {response.status_code}")
            return False
    except Exception as e:
        print_test("前端页面加载", False, str(e))
        return False


def test_authentication():
    """测试认证系统"""
    print_header("认证系统测试")

    if not ADMIN_PASSWORD:
        print_test("管理员登录", False, "未设置 APP_ADMIN_PASSWORD 环境变量（不要把真实密码写进仓库）")
        return None

    try:
        response = requests.post(
            f"{API_URL}/auth/login",
            json={"username": ADMIN_USERNAME, "password": ADMIN_PASSWORD},
            timeout=5
        )

        if response.status_code == 200:
            data = response.json()
            token = data.get('token')
            print_test("管理员登录", True, f"Token: {token[:20] if token else 'N/A'}...")
            return token
        else:
            print_test("管理员登录", False, f"状态码: {response.status_code}")
            return None
    except Exception as e:
        print_test("管理员登录", False, str(e))
        return None


def test_mqtt_broker():
    """测试MQTT代理"""
    print_header("MQTT代理测试")

    # 检查EMQX Dashboard
    try:
        response = requests.get("http://localhost:18083", timeout=5)
        if response.status_code == 200:
            print_test("EMQX Dashboard", True, "端口 18083 可访问")
        else:
            print_test("EMQX Dashboard", False, f"状态码: {response.status_code}")
    except Exception as e:
        print_test("EMQX Dashboard", False, str(e))

    # 检查MQTT端口
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(2)
        result = sock.connect_ex(('localhost', 1883))
        sock.close()

        if result == 0:
            print_test("MQTT端口 (1883)", True, "端口开放")
            return True
        else:
            print_test("MQTT端口 (1883)", False, "端口未开放")
            return False
    except Exception as e:
        print_test("MQTT端口 (1883)", False, str(e))
        return False


def test_monitoring_stack():
    """测试监控栈"""
    print_header("监控系统测试")

    results = []

    # 测试Prometheus
    try:
        response = requests.get("http://localhost:9091", timeout=5)
        status = response.status_code == 200
        print_test("Prometheus", status, f"端口 9091" + (" 可访问" if status else f" 状态码: {response.status_code}"))
        results.append(status)
    except Exception as e:
        print_test("Prometheus", False, str(e))
        results.append(False)

    # 测试Grafana
    try:
        response = requests.get("http://localhost:3000", timeout=5)
        status = response.status_code == 200
        print_test("Grafana", status, f"端口 3000" + (" 可访问" if status else f" 状态码: {response.status_code}"))
        results.append(status)
    except Exception as e:
        print_test("Grafana", False, str(e))
        results.append(False)

    return any(results)


def test_backend_health():
    """测试后端健康状态"""
    print_header("后端健康检查")

    try:
        response = requests.get(f"{API_URL}/../actuator/health", timeout=5)
        if response.status_code == 200:
            health = response.json()
            status = health.get('status', 'UNKNOWN')
            is_up = status == 'UP'
            print_test("后端健康状态", is_up, f"状态: {status}")
            return is_up
        else:
            print_test("后端健康状态", False, f"状态码: {response.status_code}")
            return False
    except Exception as e:
        print_test("后端健康状态", False, str(e))
        return False


def run():
    """运行综合测试"""
    print(f"\n{Colors.BLUE}")
    print("╔═══════════════════════════════════════════════════════════╗")
    print("║     IoT Cloud Platform - 综合集成测试                     ║")
    print("║     Comprehensive Integration Test                        ║")
    print("╚═══════════════════════════════════════════════════════════╝")
    print(f"{Colors.END}")
    print(f"测试时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")

    results = []

    # 1. 前端测试
    results.append(("Frontend", test_frontend_accessibility()))

    # 2. 认证测试
    token = test_authentication()
    results.append(("Authentication", token is not None))

    # 3. MQTT代理测试
    results.append(("MQTT Broker", test_mqtt_broker()))

    # 4. 监控系统测试
    results.append(("Monitoring", test_monitoring_stack()))

    # 5. 后端健康检查
    results.append(("Backend Health", test_backend_health()))

    # 生成报告
    print_header("测试总结")

    passed = sum(1 for _, r in results if r)
    total = len(results)

    print(f"总测试数: {total}")
    print(f"{Colors.GREEN}通过: {passed}{Colors.END}")
    print(f"{Colors.RED}失败: {total - passed}{Colors.END}")
    print(f"通过率: {(passed/total*100):.1f}%")

    if passed < total:
        print(f"\n{Colors.YELLOW}失败的测试:{Colors.END}")
        for name, status in results:
            if not status:
                print(f"  - {name}")

    print(f"\n{Colors.GREEN if passed == total else Colors.YELLOW}测试完成！{Colors.END}\n")

    return passed == total


if __name__ == "__main__":
    try:
        success = run()
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print(f"\n\n{Colors.YELLOW}测试被用户中断{Colors.END}")
        sys.exit(1)
    except Exception as e:
        print(f"\n\n{Colors.RED}测试出错: {e}{Colors.END}")
        sys.exit(1)
