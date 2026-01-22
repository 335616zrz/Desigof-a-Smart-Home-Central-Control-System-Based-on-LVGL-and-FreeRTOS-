"use strict";

// Load .env (no external deps). Safe: ignores missing file; does not override existing env.
(function loadDotEnv() {
  try {
    const p = require("path").join(__dirname, "..", ".env");
    const fs = require("fs");
    if (!fs.existsSync(p)) return;
    const raw = fs.readFileSync(p, "utf8");
    raw.split("\n").forEach((line0) => {
      let line = line0;
      if (line.endsWith("\r")) line = line.slice(0, -1); // trim CR
      if (!line || line.trimStart().startsWith("#")) return; // skip comments/blank
      const i = line.indexOf("=");
      if (i < 0) return;
      const k = line.slice(0, i).trim();
      let v = line.slice(i + 1).trim();
      if ((v.startsWith('"') && v.endsWith('"')) || (v.startsWith("'") && v.endsWith("'"))) {
        v = v.slice(1, -1);
      }
      if (!(k in process.env)) process.env[k] = v;
    });
  } catch (_) { /* ignore */ }
})();

const http = require("http");
const https = require("https");
const fs = require("fs");
const path = require("path");

const PORT = Number(process.env.PORT || 3000);
const AMAP_API_KEY = process.env.AMAP_API_KEY || "";
const DEMO = process.env.DEMO === "1";

const PUBLIC_DIR = path.join(__dirname, "..", "public");
const DATA_DIR = path.join(__dirname, "..", "data");

// Very small mime map
const MIME = {
  ".html": "text/html; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".js": "application/javascript; charset=utf-8",
  ".json": "application/json; charset=utf-8",
  ".webmanifest": "application/manifest+json; charset=utf-8",
  ".svg": "image/svg+xml",
  ".png": "image/png",
  ".jpg": "image/jpeg",
  ".jpeg": "image/jpeg",
  ".ico": "image/x-icon",
};

// In-memory cache (simple)
const cache = new Map(); // key: `${city}|${ext}` -> { data, expiresAt }

function send(res, status, body, headers = {}) {
  const buf = typeof body === "string" ? Buffer.from(body) : Buffer.from(JSON.stringify(body));
  res.writeHead(status, Object.assign({
    "Content-Type": "application/json; charset=utf-8",
    "Cache-Control": "no-store",
    "Access-Control-Allow-Origin": "*",
  }, headers));
  res.end(buf);
}

function serveStatic(req, res) {
  let pathname = "/";
  try {
    pathname = new URL(req.url, `http://${req.headers.host || 'localhost'}`).pathname || "/";
  } catch (_) { /* keep default '/' */ }
  if (pathname === "/") pathname = "/index.html";

  // Prevent path traversal
  const filePath = path.normalize(path.join(PUBLIC_DIR, pathname));
  if (!filePath.startsWith(PUBLIC_DIR)) {
    res.writeHead(403);
    return res.end("Forbidden");
  }
  fs.stat(filePath, (err, stat) => {
    if (err || !stat.isFile()) {
      res.writeHead(404);
      return res.end("Not Found");
    }
    const ext = path.extname(filePath).toLowerCase();
    const mime = MIME[ext] || "application/octet-stream";
    res.writeHead(200, { "Content-Type": mime });
    fs.createReadStream(filePath).pipe(res);
  });
}

// --- Regions loader (provinces/cities/counties) ---
const REGION_INDEX = (() => {
  const idx = {
    provinces: [], // [{code,name}]
    citiesByProv: new Map(), // prov_code -> [{code,name}]
    countiesByCity: new Map(), // city_code -> [{code,name}]
    codeToName: new Map(),
  };
  try {
    const csvPath = path.join(DATA_DIR, 'admin_divisions_2023.csv');
    const raw = fs.readFileSync(csvPath, 'utf8');
    const lines = raw.split(/\r?\n/);
    const header = lines.shift();
    // header: code,name,level,province_code,province_name,prefecture_code,prefecture_name
    for (const line0 of lines) {
      const line = line0.trim();
      if (!line) continue;
      const parts = line.split(',');
      if (parts.length < 7) continue;
      const [code, name, level, provCode, provName, prefectureCode, prefectureName] = parts;
      idx.codeToName.set(code, name);
      if (level === 'province') {
        idx.provinces.push({ code: provCode, name: provName });
        continue;
      }
      if (level === 'prefecture') {
        if (!idx.citiesByProv.has(provCode)) idx.citiesByProv.set(provCode, []);
        const arr = idx.citiesByProv.get(provCode);
        if (!arr.find(x => x.code === prefectureCode)) arr.push({ code: prefectureCode, name: prefectureName });
        continue;
      }
      if (level === 'county') {
        // group to city
        if (!idx.countiesByCity.has(prefectureCode)) idx.countiesByCity.set(prefectureCode, []);
        const arr = idx.countiesByCity.get(prefectureCode);
        arr.push({ code, name });
        // ensure city list contains at least the prefecture (for直辖市 prefecture==province)
        if (!idx.citiesByProv.has(provCode)) idx.citiesByProv.set(provCode, []);
        const carr = idx.citiesByProv.get(provCode);
        if (!carr.find(x => x.code === prefectureCode)) {
          carr.push({ code: prefectureCode, name: prefectureName || provName });
        }
      }
    }
    // sort
    idx.provinces.sort((a,b)=>a.code.localeCompare(b.code));
    for (const [k,arr] of idx.citiesByProv) arr.sort((a,b)=>a.code.localeCompare(b.code));
    for (const [k,arr] of idx.countiesByCity) arr.sort((a,b)=>a.code.localeCompare(b.code));
  } catch (e) {
    console.warn('[weather-service] 未找到行政区划数据，省市县选择功能将不可用。', e.message);
  }
  return idx;
})();

function buildAmapUrl(city, extensions = "base", output = "JSON") {
  const qp = new URLSearchParams({ key: AMAP_API_KEY, city, extensions, output });
  return `https://restapi.amap.com/v3/weather/weatherInfo?${qp.toString()}`;
}

function fetchFromAmap(city, extensions) {
  return new Promise((resolve, reject) => {
    const reqUrl = buildAmapUrl(city, extensions, "JSON");
    https.get(reqUrl, { headers: { "User-Agent": "weather-service/0.1 (+nodejs)" } }, (resp) => {
      let data = "";
      resp.setEncoding("utf8");
      resp.on("data", (chunk) => { data += chunk; });
      resp.on("end", () => {
        try {
          const json = JSON.parse(data);
          resolve(json);
        } catch (e) {
          reject(new Error("上游返回非 JSON 数据"));
        }
      });
    }).on("error", (err) => reject(err));
  });
}

function readMock(extensions) {
  const mockFile = extensions === "all" ? "sample_forecast.json" : "sample_live.json";
  const p = path.join(PUBLIC_DIR, "mock", mockFile);
  const raw = fs.readFileSync(p, "utf8");
  return JSON.parse(raw);
}

function ttlMsFor(ext) {
  return ext === "all" ? 30 * 60 * 1000 : 5 * 60 * 1000; // forecast: 30min, live: 5min
}

function handleApiWeather(req, res, query) {
  const city = (query.city || "").trim();
  const extensions = (query.extensions || "base").toLowerCase();
  const output = (query.output || "JSON").toUpperCase();

  if (!city) return send(res, 400, { status: "0", info: "缺少必填参数 city（adcode）" });
  if (!(extensions === "base" || extensions === "all")) {
    return send(res, 400, { status: "0", info: "参数 extensions 仅支持 base 或 all" });
  }
  if (output !== "JSON") {
    return send(res, 400, { status: "0", info: "仅支持 JSON 输出" });
  }

  const key = `${city}|${extensions}`;
  const now = Date.now();
  const hit = cache.get(key);
  if (hit && hit.expiresAt > now) {
    return send(res, 200, hit.data, { "X-Cache": "HIT" });
  }

  (async () => {
    try {
      let data;
      if (DEMO && !AMAP_API_KEY) {
        data = readMock(extensions);
      } else {
        if (!AMAP_API_KEY) {
          return send(res, 501, { status: "0", info: "未配置 AMAP_API_KEY，请设置环境变量或启用 DEMO=1 使用演示数据。" });
        }
        data = await fetchFromAmap(city, extensions);
      }
      cache.set(key, { data, expiresAt: now + ttlMsFor(extensions) });
      send(res, 200, data, { "X-Cache": "MISS" });
    } catch (err) {
      send(res, 502, { status: "0", info: "上游请求失败", error: String((err && err.message) || err) });
    }
  })();
}

function router(req, res) {
  const u = new URL(req.url, `http://${req.headers.host || 'localhost'}`);
  const pathname = u.pathname;
  const query = Object.fromEntries(u.searchParams.entries());

  if (req.method === "GET" && pathname === "/healthz") {
    res.writeHead(200, { "Content-Type": "text/plain; charset=utf-8" });
    return res.end("ok");
  }
  if (req.method === "GET" && (pathname === "/favicon.ico" || pathname === "/apple-touch-icon.png")) {
    const png = Buffer.from(
      "iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAACXBIWXMAAAsSAAALEgHS3X78AAAAH0lEQVQ4jWNgGAWjYBSMglEwCkbB/0EMTQy0CjEAAAJmAAH9Tqz8AAAAAElFTkSuQmCC",
      "base64"
    );
    res.writeHead(200, {
      "Content-Type": "image/png",
      "Cache-Control": "public, max-age=86400"
    });
    return res.end(png);
  }
  if (req.method === 'GET' && pathname === '/api/regions/provinces') {
    return send(res, 200, { provinces: REGION_INDEX.provinces || [] });
  }
  if (req.method === 'GET' && pathname === '/api/regions/cities') {
    const prov = (query.province || '').trim();
    const cities = (REGION_INDEX.citiesByProv.get(prov) || []);
    return send(res, 200, { cities });
  }
  if (req.method === 'GET' && pathname === '/api/regions/counties') {
    const city = (query.city || '').trim();
    const counties = (REGION_INDEX.countiesByCity.get(city) || []);
    return send(res, 200, { counties });
  }
  if (req.method === "GET" && pathname === "/api/weather") {
    return handleApiWeather(req, res, query);
  }
  return serveStatic(req, res);
}

const server = http.createServer(router);
server.on('error', (err) => {
  if (err && err.code === 'EADDRINUSE') {
    console.error(`[weather-service] 端口 ${PORT} 已被占用。可选择：\n` +
      `  1) 改用其他端口运行：PORT=3001 node src/server.js\n` +
      `  2) 结束占用进程后再启动（见 README 中排查命令）。`);
  } else {
    console.error('[weather-service] 启动失败:', err);
  }
  process.exit(1);
});

server.listen(PORT, () => {
  const addr = server.address();
  const actualPort = (addr && typeof addr === 'object') ? addr.port : PORT;
  console.log(`[weather-service] listening on http://localhost:${actualPort}`);
  if (!AMAP_API_KEY) {
    console.log("[weather-service] AMAP_API_KEY 未设置。可设置 DEMO=1 使用演示数据。");
  }
});
