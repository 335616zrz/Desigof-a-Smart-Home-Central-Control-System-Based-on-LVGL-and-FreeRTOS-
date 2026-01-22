import asyncio
import base64
import json
import os
import sys
import uuid

import websockets


COSYVOICE_API_KEY = os.environ.get("COSYVOICE_API_KEY")
MODEL = os.environ.get("COSYVOICE_MODEL", "cosyvoice-v3-plus")
VOICE = os.environ.get("COSYVOICE_VOICE", "longhuhu_v3")
FORMAT = os.environ.get("COSYVOICE_FORMAT", "pcm")
SAMPLE_RATE = int(os.environ.get("COSYVOICE_SAMPLE_RATE", 24000))

TEXT = "你好，很高兴见到你！欢迎来到 CosyVoice 测试。"


async def main():
    if not COSYVOICE_API_KEY:
        print("COSYVOICE_API_KEY 未设置", file=sys.stderr)
        return

    headers = {
        "Authorization": f"bearer {COSYVOICE_API_KEY}",
        "X-DashScope-DataInspection": "enable",
    }

    async with websockets.connect(
        "wss://dashscope.aliyuncs.com/api-ws/v1/inference",
        extra_headers=headers,
        max_size=None,
    ) as ws:
        task_id = uuid.uuid4().hex
        run_task = {
            "header": {
                "action": "run-task",
                "task_id": task_id,
                "streaming": "duplex",
            },
            "payload": {
                "task_group": "audio",
                "task": "tts",
                "function": "SpeechSynthesizer",
                "model": MODEL,
                "parameters": {
                    "text_type": "PlainText",
                    "voice": VOICE,
                    "format": FORMAT,
                    "sample_rate": SAMPLE_RATE,
                },
                "input": {},
            },
        }
        await ws.send(json.dumps(run_task))

        async for message in ws:
            if isinstance(message, bytes):
                chunk = base64.b64encode(message).decode()
                print(f"chunk {len(message)} bytes -> {chunk[:32]}...")
                continue

            payload = json.loads(message)
            event = payload.get("header", {}).get("event")
            print("event", event)

            if event == "task-started":
                await ws.send(
                    json.dumps(
                        {
                            "header": {
                                "action": "continue-task",
                                "task_id": task_id,
                                "streaming": "duplex",
                            },
                            "payload": {
                                "input": {
                                    "text": TEXT,
                                }
                            },
                        }
                    )
                )
                await ws.send(
                    json.dumps(
                        {
                            "header": {
                                "action": "finish-task",
                                "task_id": task_id,
                                "streaming": "duplex",
                            },
                            "payload": {"input": {}},
                        }
                    )
                )
            elif event == "task-finished":
                break


if __name__ == "__main__":
    asyncio.run(main())
