#!/usr/bin/env python3
"""
MQTT集成测试模块
"""

import paho.mqtt.client as mqtt
import json
import time
import random
import sys


class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    END = '\033[0m'


# MQTT配置
MQTT_BROKER = "localhost"
MQTT_PORT = 1883
MQTT_USERNAME = "iot_device"
MQTT_PASSWORD = "iot_device_pass"
DEVICE_ID = "esp32-sht40"
TOPIC = f"devices/{DEVICE_ID}/state"


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"{Colors.GREEN}✓ Connected to MQTT Broker at {MQTT_BROKER}:{MQTT_PORT}{Colors.END}")
    else:
        print(f"{Colors.RED}✗ Failed to connect, return code {rc}{Colors.END}")


def on_publish(client, userdata, mid):
    print(f"{Colors.GREEN}✓ Message published (mid: {mid}){Colors.END}")


def run():
    """运行MQTT测试"""
    print(f"\n{Colors.BLUE}{'='*60}")
    print("MQTT Integration Test")
    print(f"{'='*60}{Colors.END}\n")

    # 创建MQTT客户端
    client = mqtt.Client(client_id=f"{DEVICE_ID}_test")
    client.on_connect = on_connect
    client.on_publish = on_publish

    # 设置凭证
    try:
        client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
        print(f"Using credentials: {MQTT_USERNAME}")
    except Exception as e:
        print(f"{Colors.YELLOW}Warning: Could not set credentials: {e}{Colors.END}")
        print("Trying anonymous connection...")

    # 连接到代理
    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_start()
        time.sleep(2)  # 等待连接

        # 发布测试数据
        for i in range(3):
            temperature = round(20 + random.uniform(-5, 10), 2)
            humidity = round(50 + random.uniform(-10, 20), 2)

            payload = {
                "temperature": temperature,
                "humidity": humidity
            }

            result = client.publish(TOPIC, json.dumps(payload), qos=1)
            print(f"\n📡 Publishing to {TOPIC}:")
            print(f"   Temperature: {temperature}°C")
            print(f"   Humidity: {humidity}%")

            time.sleep(2)

        client.loop_stop()
        client.disconnect()
        print(f"\n{Colors.GREEN}✓ MQTT test completed successfully{Colors.END}")
        return True

    except Exception as e:
        print(f"\n{Colors.RED}✗ MQTT test error: {e}{Colors.END}")
        return False


if __name__ == "__main__":
    try:
        success = run()
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print(f"\n\n{Colors.YELLOW}⚠️  测试被用户中断{Colors.END}")
        sys.exit(1)
