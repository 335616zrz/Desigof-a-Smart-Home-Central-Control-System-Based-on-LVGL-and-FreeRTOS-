#!/usr/bin/env python3
"""
Simple HTTP service to expose recent Caddy access logs for the music server.
Returns JSON with the latest events and aggregated statistics per IP/user.
"""

from __future__ import annotations

import json
import os
import threading
import time
from collections import defaultdict, deque
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any, Deque, Dict, List
from urllib.parse import parse_qs, urlparse

LOG_PATH = Path("/var/log/caddy/music-access.log")
DEFAULT_LIMIT = 120  # number of entries to keep in API response
CACHE_TTL = 1.5  # seconds

_cache_lock = threading.Lock()
_cache_data: Dict[str, Any] | None = None
_cache_time: float = 0.0


def tail_json_lines(path: Path, limit: int) -> List[Dict[str, Any]]:
    if not path.exists():
        return []
    records: Deque[str] = deque(maxlen=limit * 4)
    with path.open("r", encoding="utf-8") as fh:
        for line in fh:
            stripped = line.strip()
            if stripped:
                records.append(stripped)
    parsed: List[Dict[str, Any]] = []
    for line in records:
        try:
            parsed.append(json.loads(line))
        except json.JSONDecodeError:
            continue
    return parsed[-limit:]


def build_payload(limit: int = DEFAULT_LIMIT) -> Dict[str, Any]:
    raw_entries = tail_json_lines(LOG_PATH, limit)
    entries = []
    per_ip: Dict[str, Dict[str, Any]] = defaultdict(
        lambda: {
            "ip": "",
            "user": None,
            "hits": 0,
            "downloads": 0,
            "last_uri": "",
            "last_ts": 0.0,
        }
    )

    for item in raw_entries:
        req = item.get("request", {})
        ip = req.get("remote_ip") or req.get("client_ip") or "-"
        user_id = item.get("user_id") or None
        ts = item.get("ts") or time.time()
        uri = req.get("uri") or ""
        method = req.get("method") or ""
        status = item.get("status")
        duration = item.get("duration")

        entries.append(
            {
                "ts": ts,
                "iso": datetime.fromtimestamp(ts, timezone.utc).astimezone().isoformat(),
                "ip": ip,
                "method": method,
                "uri": uri,
                "status": status,
                "duration_ms": round((duration or 0) * 1000, 2),
                "user": user_id,
                "bytes": item.get("size", 0),
            }
        )

        tracker = per_ip[ip]
        tracker["ip"] = ip
        tracker["user"] = user_id or tracker["user"]
        tracker["hits"] += 1
        if "download=" in uri:
            tracker["downloads"] += 1
        if ts >= tracker["last_ts"]:
            tracker["last_ts"] = ts
            tracker["last_uri"] = uri

    ordered_ips = sorted(
        per_ip.values(),
        key=lambda x: (x["hits"], x["last_ts"]),
        reverse=True,
    )

    return {
        "generated_at": datetime.now(timezone.utc).astimezone().isoformat(),
        "entries": entries[-limit:],
        "per_ip": ordered_ips,
        "unique_ips": len(per_ip),
        "limit": limit,
    }


def get_cached_payload(limit: int = DEFAULT_LIMIT) -> Dict[str, Any]:
    global _cache_data, _cache_time
    now = time.time()
    with _cache_lock:
        if _cache_data is None or now - _cache_time > CACHE_TTL:
            _cache_data = build_payload(limit)
            _cache_time = now
        return _cache_data


class LogRequestHandler(BaseHTTPRequestHandler):
    server_version = "MusicLogAPI/1.0"

    def do_GET(self) -> None:  # noqa: N802
        parsed = urlparse(self.path)
        path = parsed.path or "/"
        if path.endswith("/healthz") or path == "/healthz":
            self.respond_json({"status": "ok", "ts": time.time()})
            return
        if path.endswith("/logs") or path == "/logs":
            limit = DEFAULT_LIMIT
            try:
                params = parse_qs(parsed.query or "")
                if "limit" in params:
                    limit_value = int(params["limit"][0])
                    limit = max(10, min(500, limit_value))
            except ValueError:
                limit = DEFAULT_LIMIT
            payload = get_cached_payload(limit)
            self.respond_json(payload)
            return
        self.send_error(404, "Not Found")

    def log_message(self, format: str, *args: Any) -> None:  # noqa: A003
        # Silence default stdout logging
        return

    def respond_json(self, payload: Dict[str, Any]) -> None:
        encoded = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(encoded)))
        self.end_headers()
        self.wfile.write(encoded)


def run() -> None:
    port = int(os.environ.get("LOG_API_PORT", "9000"))
    address = ("127.0.0.1", port)
    httpd = ThreadingHTTPServer(address, LogRequestHandler)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        httpd.server_close()


if __name__ == "__main__":
    run()
