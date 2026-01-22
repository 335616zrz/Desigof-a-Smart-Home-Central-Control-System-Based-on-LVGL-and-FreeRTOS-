#!/usr/bin/env python3
"""
API测试模块 - 测试所有REST API端点
"""

import os
import requests
import json
import sys
from typing import Dict, Any, Optional

BASE_URL = "http://localhost:8080/api"
_token: Optional[str] = None
ADMIN_USERNAME = os.getenv("APP_ADMIN_USERNAME", "admin")
ADMIN_PASSWORD = os.getenv("APP_ADMIN_PASSWORD", "")


class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    END = '\033[0m'


def print_test(test_name: str):
    """打印测试标题"""
    print(f"\n{Colors.BLUE}{'='*60}")
    print(f"TEST: {test_name}")
    print(f"{'='*60}{Colors.END}")


def print_result(success: bool, message: str, data: Any = None):
    """打印测试结果"""
    status = f"{Colors.GREEN}✅ PASS" if success else f"{Colors.RED}❌ FAIL"
    print(f"{status}: {message}{Colors.END}")
    if data and isinstance(data, dict):
        print(json.dumps(data, indent=2, ensure_ascii=False))
    elif data:
        print(data)


def get_headers():
    """获取带认证令牌的请求头"""
    return {
        "Authorization": f"Bearer {_token}",
        "Content-Type": "application/json"
    }


def test_auth():
    """测试认证端点"""
    global _token

    print_test("Authentication - Login")
    if not ADMIN_PASSWORD:
        print_result(False, "Missing APP_ADMIN_PASSWORD env (do not hardcode real passwords in repo)")
        return False

    response = requests.post(
        f"{BASE_URL}/auth/login",
        json={"username": ADMIN_USERNAME, "password": ADMIN_PASSWORD}
    )

    if response.status_code == 200:
        data = response.json()
        _token = data.get('token')
        print_result(True, "Login successful", {"token": f"{_token[:20]}..."})
        return True
    else:
        print_result(False, f"Login failed with status {response.status_code}", response.text)
        return False


def test_devices():
    """测试设备管理端点"""

    print_test("Devices - List All")
    response = requests.get(f"{BASE_URL}/devices", headers=get_headers())
    if response.status_code == 200:
        devices = response.json()
        print_result(True, f"Found {len(devices)} devices",
                     {f"device_{i}": d.get('name') for i, d in enumerate(devices[:3])})
    else:
        print_result(False, f"Failed to list devices: {response.status_code}", response.text)
        return False

    return True


def test_alerts():
    """测试告警管理端点"""

    print_test("Alerts - List All")
    response = requests.get(f"{BASE_URL}/alerts", headers=get_headers())
    if response.status_code == 200:
        data = response.json()
        total = data.get('total', 0)
        print_result(True, f"Found {total} alerts")
        return True
    else:
        print_result(False, f"Failed to list alerts: {response.status_code}", response.text)
        return False


def test_alert_rules():
    """测试告警规则管理"""

    print_test("Alert Rules - List All")
    response = requests.get(f"{BASE_URL}/alert-rules", headers=get_headers())
    if response.status_code == 200:
        rules = response.json()
        print_result(True, f"Found {len(rules)} alert rules")
        return True
    else:
        print_result(False, f"Failed to list rules: {response.status_code}", response.text)
        return False


def test_users():
    """测试用户管理端点"""

    print_test("Users - List All")
    response = requests.get(f"{BASE_URL}/users", headers=get_headers())
    if response.status_code == 200:
        data = response.json()
        total = data.get('total', 0)
        print_result(True, f"Found {total} users")
        return True
    else:
        print_result(False, f"Failed to list users: {response.status_code}", response.text)
        return False


def test_telemetry():
    """测试遥测数据端点"""

    print_test("Telemetry - Query Recent Data")
    response = requests.get(
        f"{BASE_URL}/telemetry",
        params={"deviceId": "esp32-demo-001", "limit": 10},
        headers=get_headers()
    )
    if response.status_code == 200:
        print_result(True, "Retrieved telemetry data")
        return True
    else:
        print_result(False, f"Failed to query telemetry: {response.status_code}", response.text)
        return False


def run():
    """运行所有API测试"""
    print(f"\n{Colors.BLUE}{'='*60}")
    print("IoT Cloud Platform - API Test Suite")
    print(f"{'='*60}{Colors.END}\n")

    # 测试认证
    if not test_auth():
        print(f"\n{Colors.RED}❌ 认证失败，无法继续测试{Colors.END}")
        return False

    # 运行所有测试
    tests = [
        ("Devices", test_devices),
        ("Alerts", test_alerts),
        ("Alert Rules", test_alert_rules),
        ("Users", test_users),
        ("Telemetry", test_telemetry)
    ]

    results = []
    for name, test_func in tests:
        try:
            result = test_func()
            results.append((name, result))
        except Exception as e:
            print_result(False, f"{name} test error: {e}")
            results.append((name, False))

    # 输出结果
    passed = sum(1 for _, r in results if r)
    total = len(results)

    print(f"\n{Colors.BLUE}{'='*60}")
    print(f"API Tests: {passed}/{total} passed")
    print(f"{'='*60}{Colors.END}\n")

    return passed == total


if __name__ == "__main__":
    try:
        success = run()
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print(f"\n\n{Colors.YELLOW}⚠️  测试被用户中断{Colors.END}")
        sys.exit(1)
    except Exception as e:
        print(f"\n\n{Colors.RED}❌ 测试出错: {e}{Colors.END}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
