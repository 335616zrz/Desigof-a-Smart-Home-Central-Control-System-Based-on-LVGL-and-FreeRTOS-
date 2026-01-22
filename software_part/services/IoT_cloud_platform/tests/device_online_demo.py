#!/usr/bin/env python3
"""
设备上线演示脚本 - 最简版本
"""

import os
import paho.mqtt.client as mqtt
import json
import time
import sys
import random

MQTT_BROKER = os.getenv("MQTT_BROKER", "localhost")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
# Do not hardcode credentials in repo; pass via env when running this demo.
MQTT_USERNAME = os.getenv("MQTT_USERNAME", "")
MQTT_PASSWORD = os.getenv("MQTT_PASSWORD", "")
DEVICE_ID = "test-device-001"
TOPIC = f"devices/{DEVICE_ID}/state"

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"✅ 连接成功: {MQTT_BROKER}:{MQTT_PORT}")
        print(f"   设备: {DEVICE_ID}")
    else:
        print(f"❌ 连接失败 (code: {rc})")
        sys.exit(1)

def on_publish(client, userdata, mid):
    print(f"✅ 消息已发布 (id: {mid})")

def main():
    print("=" * 60)
    print("设备上线演示")
    print("=" * 60)

    if not MQTT_USERNAME or not MQTT_PASSWORD:
        print("❌ 请先设置 MQTT_USERNAME / MQTT_PASSWORD 环境变量（不要把真实密码写进仓库）")
        sys.exit(1)

    client = mqtt.Client(client_id=f"{DEVICE_ID}_demo")
    client.on_connect = on_connect
    client.on_publish = on_publish
    client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)

    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_start()
        time.sleep(2)

        print(f"\n📤 发送 3 条消息到: {TOPIC}\n")

        for i in range(3):
            temp = round(20 + random.uniform(-5, 10), 2)
            hum = round(50 + random.uniform(-10, 20), 2)

            payload = {
                "temperature": temp,
                "humidity": hum,
                "timestamp": int(time.time())
            }

            print(f"消息 {i+1}: 温度={temp}°C, 湿度={hum}%")
            client.publish(TOPIC, json.dumps(payload), qos=1)
            time.sleep(3)

        print("\n✅ 完成! 设备状态应该已变为 ONLINE")
        print(f"\n验证命令:")
        print(
            "docker exec iot-mysql mysql -u<MYSQL_USER> -p<MYSQL_PASSWORD> <MYSQL_DATABASE> "
            f"-e \"SELECT device_id, status FROM devices WHERE device_id = '{DEVICE_ID}';\" "
            "2>&1 | grep -v Warning"
        )

    except Exception as e:
        print(f"❌ 错误: {e}")
    finally:
        client.loop_stop()
        client.disconnect()

    print("=" * 60)

if __name__ == "__main__":
    main()
