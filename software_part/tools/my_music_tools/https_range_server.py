# https_range_server.py
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import ssl, os, re

ROOT = os.path.dirname(__file__)

class RangeHandler(SimpleHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def do_GET(self):
        path = self.translate_path(self.path)
        if not os.path.isfile(path):
            self.send_error(404); return
        total = os.path.getsize(path)

        start, end = 0, total - 1
        rng = self.headers.get("Range")
        if rng:
            m = re.match(r"bytes=(\d+)-(\d+)?", rng)
            if m:
                start = int(m.group(1))
                if m.group(2) is not None:
                    end = int(m.group(2))
            self.send_response(206, "Partial Content")
            self.send_header("Accept-Ranges", "bytes")
            self.send_header("Content-Range", f"bytes {start}-{end}/{total}")
        else:
            self.send_response(200)

        clen = end - start + 1
        # 视你的文件类型调整
        self.send_header("Content-Type", "audio/mpeg")
        self.send_header("Content-Length", str(clen))
        self.send_header("Connection", "keep-alive")   # 关键：不主动关
        self.end_headers()

        try:
            with open(path, "rb") as f:
                f.seek(start)
                remain = clen
                # 小块写，减少一次写就被对端关掉的概率
                while remain > 0:
                    chunk = f.read(min(64*1024, remain))
                    if not chunk:
                        break
                    self.wfile.write(chunk)
                    remain -= len(chunk)
                self.wfile.flush()
        except (BrokenPipeError, ssl.SSLEOFError):
            # 客户端提前断开，忽略即可
            pass

if __name__ == "__main__":
    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)  # 现代写法
    ctx.load_cert_chain(certfile=os.path.join(ROOT, "../server.crt"),
                        keyfile=os.path.join(ROOT, "../server.key"))
    httpd = ThreadingHTTPServer(("0.0.0.0", 8443), RangeHandler)
    httpd.socket = ctx.wrap_socket(httpd.socket, server_side=True)
    print("Serving HTTPS with Range at https://0.0.0.0:8443")
    httpd.serve_forever()

