#!/usr/bin/env python3
"""
IoT Cloud Platform - 统一测试套件
Unified Test Suite - API, E2E, Integration Tests
"""

import argparse
import sys
import importlib.util

# 动态导入测试模块
def import_test_module(module_path, module_name):
    """动态导入测试模块"""
    try:
        spec = importlib.util.spec_from_file_location(module_name, module_path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module
    except Exception as e:
        print(f"⚠️  无法导入 {module_name}: {e}")
        return None


def main():
    parser = argparse.ArgumentParser(description='IoT Cloud Platform 统一测试套件')
    parser.add_argument('--type', choices=['all', 'api', 'e2e', 'integration', 'mqtt'],
                        default='all', help='测试类型')
    parser.add_argument('--browser', choices=['selenium', 'playwright', 'none'],
                        default='none', help='E2E测试浏览器')
    args = parser.parse_args()

    print("="*70)
    print("   IoT Cloud Platform - 统一测试套件")
    print("   Unified Test Suite")
    print("="*70)
    print(f"\n测试类型: {args.type}")
    if args.type in ['all', 'e2e']:
        print(f"浏览器: {args.browser}")
    print("")

    # 导入测试模块
    test_modules = {}

    # API测试
    if args.type in ['all', 'api']:
        from .api import test_api_endpoints
        test_modules['api'] = test_api_endpoints

    # 集成测试
    if args.type in ['all', 'integration']:
        from .integration import test_comprehensive
        test_modules['integration'] = test_comprehensive

    # MQTT测试
    if args.type in ['all', 'mqtt']:
        from .integration import test_mqtt_client
        test_modules['mqtt'] = test_mqtt_client

    # E2E测试
    if args.type in ['all', 'e2e'] and args.browser != 'none':
        if args.browser == 'selenium':
            from .e2e import test_selenium_browser
            test_modules['e2e'] = test_selenium_browser
        elif args.browser == 'playwright':
            from .e2e import test_playwright_browser
            test_modules['e2e'] = test_playwright_browser

    # 执行测试
    results = []
    for name, module in test_modules.items():
        try:
            print(f"\n{'='*70}")
            print(f"  执行 {name.upper()} 测试")
            print(f"{'='*70}\n")
            result = module.run()
            results.append((name, result))
        except Exception as e:
            print(f"❌ {name} 测试失败: {e}")
            results.append((name, False))

    # 输出总结
    print("\n" + "="*70)
    print("  测试总结")
    print("="*70)
    for name, result in results:
        status = "✅ 通过" if result else "❌ 失败"
        print(f"{name.upper():20s} {status}")

    passed = sum(1 for _, r in results if r)
    total = len(results)
    print(f"\n总计: {passed}/{total} 通过")

    return 0 if passed == total else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n\n⚠️  测试被用户中断")
        sys.exit(1)
    except Exception as e:
        print(f"\n\n❌ 测试出错: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
