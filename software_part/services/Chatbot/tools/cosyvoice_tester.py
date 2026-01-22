"""CosyVoice WebSocket 语音合成测试工具

使用 Tkinter 提供一个简单 UI，输入待合成文本后调用 CosyVoice WebSocket
接口，将返回的 PCM 音频播放出来（若安装了 simpleaudio），同时也可
选择保存为 WAV 文件。

依赖：
    - Python 3.9+
    - websockets (pip install websockets)
    - simpleaudio (可选，pip install simpleaudio)

在启动前，请设置环境变量 COSYVOICE_API_KEY 或在界面中填写 API Key。
"""

from __future__ import annotations

import asyncio
import base64
import json
import os
import ssl
import tempfile
import threading
import time
import uuid
import wave
from pathlib import Path
from typing import List

import tkinter as tk
from tkinter import filedialog, messagebox, scrolledtext

try:
    import simpleaudio as sa  # type: ignore
except ImportError:  # pragma: no cover - 提示用户安装 simpleaudio
    sa = None

import websockets


COSYVOICE_URL = "wss://dashscope.aliyuncs.com/api-ws/v1/inference"


class CosyVoiceClient:
    def __init__(self, api_key: str, model: str = "cosyvoice-v3", voice: str = "longhuhu_v3",
                fmt: str = "pcm", sample_rate: int = 24000) -> None:
        self.api_key = api_key
        self.model = model
        self.voice = voice
        self.format = fmt
        self.sample_rate = sample_rate

    async def synthesize(self, text: str) -> tuple[bytes, int]:
        if not text.strip():
            raise ValueError("待合成文本不能为空")

        headers = {
            "Authorization": f"bearer {self.api_key}",
            "X-DashScope-DataInspection": "enable",
        }

        ssl_context = ssl.create_default_context()
        task_id = uuid.uuid4().hex

        chunks: List[bytes] = []
        playback_sample_rate = self.sample_rate

        async with websockets.connect(
            COSYVOICE_URL,
            additional_headers=headers,
            ssl=ssl_context,
            max_size=None,
        ) as ws:
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
                    "model": self.model,
                    "parameters": {
                        "text_type": "PlainText",
                        "voice": self.voice,
                        "format": self.format,
                        "sample_rate": self.sample_rate,
                    },
                    "input": {},
                },
            }

            await ws.send(json.dumps(run_task))

            async for raw in ws:
                if isinstance(raw, (bytes, bytearray)):
                    if raw:
                        chunks.append(bytes(raw))
                    continue

                message = json.loads(raw)
                event = message.get("header", {}).get("event")

                if event == "task-started":
                    await self._send_text(ws, task_id, text)
                elif event == "result-generated":
                    usage = message.get("payload", {}).get("usage", {})
                    print("[CosyVoice] usage", usage)
                elif event == "task-finished":
                    payload = message.get("payload", {})
                    meta_rate = payload.get("output", {}).get("sentence", {}).get("sample_rate")
                    if isinstance(meta_rate, int) and meta_rate > 0:
                        playback_sample_rate = meta_rate
                    break
                elif event == "task-failed":
                    error_message = message.get("header", {}).get("error_message", "任务失败")
                    raise RuntimeError(f"CosyVoice 任务失败：{error_message}")

        if not chunks:
            raise RuntimeError("未收到任何音频数据，请检查模型权限或输入内容")

        audio_bytes = b"".join(chunks)
        return audio_bytes, playback_sample_rate

    async def _send_text(self, ws: websockets.WebSocketClientProtocol, task_id: str, text: str) -> None:
        pieces = self._split_text(text)
        for piece in pieces:
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
                                "text": piece,
                            }
                        },
                    }
                )
            )
        await asyncio.sleep(0.2)
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

    @staticmethod
    def _split_text(text: str, limit: int = 2000) -> List[str]:
        if len(text) <= limit:
            return [text]
        chunks: List[str] = []
        start = 0
        while start < len(text):
            chunks.append(text[start:start + limit])
            start += limit
        return chunks


class CosyVoiceApp:
    def __init__(self) -> None:
        self.root = tk.Tk()
        self.root.title("CosyVoice 语音合成测试")
        self.root.geometry("640x520")

        self.api_key_var = tk.StringVar(value=os.environ.get("COSYVOICE_API_KEY", ""))
        self.model_var = tk.StringVar(value=os.environ.get("COSYVOICE_MODEL", "cosyvoice-v3"))
        self.voice_var = tk.StringVar(value=os.environ.get("COSYVOICE_VOICE", "longhuhu_v3"))
        self.sample_rate_var = tk.StringVar(value=os.environ.get("COSYVOICE_SAMPLE_RATE", "24000"))

        self._build_ui()

    def _build_ui(self) -> None:
        pad = 8
        frm = tk.Frame(self.root)
        frm.pack(fill=tk.BOTH, expand=True, padx=pad, pady=pad)

        tk.Label(frm, text="API Key:").grid(row=0, column=0, sticky="w")
        tk.Entry(frm, textvariable=self.api_key_var, show="*", width=60).grid(row=0, column=1, sticky="we")

        tk.Label(frm, text="模型(model):").grid(row=1, column=0, sticky="w")
        tk.Entry(frm, textvariable=self.model_var).grid(row=1, column=1, sticky="we")

        tk.Label(frm, text="音色(voice):").grid(row=2, column=0, sticky="w")
        tk.Entry(frm, textvariable=self.voice_var).grid(row=2, column=1, sticky="we")

        tk.Label(frm, text="采样率(sample_rate):").grid(row=3, column=0, sticky="w")
        tk.Entry(frm, textvariable=self.sample_rate_var).grid(row=3, column=1, sticky="we")

        tk.Label(frm, text="待合成文本:").grid(row=4, column=0, sticky="nw")
        self.text_box = scrolledtext.ScrolledText(frm, wrap=tk.WORD, height=10)
        self.text_box.grid(row=4, column=1, sticky="nsew")

        self.status_var = tk.StringVar(value="请填写文本后点击“合成并播放”。")
        tk.Label(frm, textvariable=self.status_var, fg="#2563eb").grid(row=5, column=0, columnspan=2, sticky="w", pady=(pad, 0))

        btn_frame = tk.Frame(frm)
        btn_frame.grid(row=6, column=1, sticky="e", pady=(pad, 0))

        tk.Button(btn_frame, text="保存 WAV", command=self.save_wav).pack(side=tk.RIGHT)
        tk.Button(btn_frame, text="合成并播放", command=self.start_synthesis).pack(side=tk.RIGHT, padx=(0, pad))

        frm.columnconfigure(1, weight=1)
        frm.rowconfigure(4, weight=1)

    def start(self) -> None:
        self.root.mainloop()

    def log(self, message: str) -> None:
        print(message)
        self.status_var.set(message)

    def start_synthesis(self) -> None:
        api_key = self.api_key_var.get().strip()
        if not api_key:
            messagebox.showwarning("提示", "请填写有效的 API Key")
            return

        text = self.text_box.get("1.0", tk.END).strip()
        if not text:
            messagebox.showwarning("提示", "请输入需要合成的文本")
            return

        try:
            sample_rate = int(self.sample_rate_var.get())
        except ValueError:
            messagebox.showerror("错误", "采样率必须是整数")
            return

        client = CosyVoiceClient(
            api_key=api_key,
            model=self.model_var.get().strip() or "cosyvoice-v3",
            voice=self.voice_var.get().strip() or "longhuhu_v3",
            fmt="pcm",
            sample_rate=sample_rate,
        )

        self.log("开始请求 CosyVoice…")

        def worker() -> None:
            try:
                audio_bytes, sr = asyncio.run(client.synthesize(text))
            except Exception as exc:  # pylint: disable=broad-except
                def report_error(message: str = str(exc)) -> None:
                    messagebox.showerror("合成失败", message)
                    self.log(message)

                self.root.after(0, report_error)
                return

            self.root.after(0, lambda: self.log("合成成功，正在播放…"))
            self.root.after(0, lambda: self.play_audio(audio_bytes, sr))

        threading.Thread(target=worker, daemon=True).start()

    def play_audio(self, audio_bytes: bytes, sample_rate: int) -> None:
        if sa:
            try:
                play_obj = sa.play_buffer(audio_bytes, 1, 2, sample_rate)
                play_obj.wait_done()
                self.log("播放完成")
                return
            except Exception as exc:  # pragma: no cover
                self.log(f"simpleaudio 播放失败：{exc}")

        # simpleaudio 未安装，退化为保存临时 WAV
        tmp_path = Path(tempfile.gettempdir()) / f"cosyvoice_{int(time.time())}.wav"
        self._write_wav(tmp_path, audio_bytes, sample_rate)
        self.log(f"已保存到 {tmp_path}，请使用播放器打开收听")

    def save_wav(self) -> None:
        api_key = self.api_key_var.get().strip()
        if not api_key:
            messagebox.showwarning("提示", "请先填写 API Key，再输入文本")
            return

        text = self.text_box.get("1.0", tk.END).strip()
        if not text:
            messagebox.showwarning("提示", "请输入需要合成的文本")
            return

        file_path = filedialog.asksaveasfilename(
            title="保存 WAV", defaultextension=".wav", filetypes=[("WAV 文件", "*.wav")]
        )
        if not file_path:
            return

        self.log("正在生成 WAV 文件…")

        def worker() -> None:
            try:
                client = CosyVoiceClient(
                    api_key=api_key,
                    model=self.model_var.get().strip() or "cosyvoice-v3",
                    voice=self.voice_var.get().strip() or "longhuhu_v3",
                    fmt="pcm",
                    sample_rate=int(self.sample_rate_var.get()),
                )
                audio_bytes, sr = asyncio.run(client.synthesize(text))
                self._write_wav(Path(file_path), audio_bytes, sr)
                self.root.after(0, lambda: self.log(f"已保存到 {file_path}"))
            except Exception as exc:  # pylint: disable=broad-except
                def report_error(message: str = str(exc)) -> None:
                    messagebox.showerror("合成失败", message)
                    self.log(message)

                self.root.after(0, report_error)

        threading.Thread(target=worker, daemon=True).start()

    @staticmethod
    def _write_wav(path: Path, audio_bytes: bytes, sample_rate: int) -> None:
        # 将 PCM int16 数据写入 WAV
        with wave.open(str(path), "wb") as wav:
            wav.setnchannels(1)
            wav.setsampwidth(2)
            wav.setframerate(sample_rate)
            wav.writeframes(audio_bytes)


def main() -> None:
    app = CosyVoiceApp()
    app.start()


if __name__ == "__main__":
    main()
