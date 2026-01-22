const statusEl = document.getElementById("status");
const totalCountEl = document.getElementById("totalCount");
const filteredCountEl = document.getElementById("filteredCount");
const duplicateCountEl = document.getElementById("duplicateCount");
const searchInput = document.getElementById("search");
const reloadBtn = document.getElementById("reload");
const showDuplicates = document.getElementById("showDuplicates");
const tableBody = document.getElementById("entries");
const audioEl = document.getElementById("audio");
const currentTitleEl = document.getElementById("currentTitle");
const rowTemplate = document.getElementById("row-template");
const visualizerCanvas = document.getElementById("visualizer");
const visualizerCtx = visualizerCanvas ? visualizerCanvas.getContext("2d") : null;
const refreshLogsBtn = document.getElementById("refreshLogs");
const logMetaEl = document.getElementById("logMeta");
const ipSummaryBody = document.getElementById("ipSummary");
const logStreamEl = document.getElementById("logStream");

const state = {
  records: [],
  sortKey: "title",
  sortDir: 1,
  activeRow: null,
};

let audioContext = null;
let analyserNode = null;
let analyserData = null;
let sourceNode = null;
let visualizerFrame = null;
let logTimer = null;

if (visualizerCanvas) {
  resizeVisualizer();
  stopVisualization();
  window.addEventListener("resize", resizeVisualizer);
}

document.querySelectorAll("thead th[data-sort]").forEach((th) => {
  th.addEventListener("click", () => {
    const key = th.dataset.sort;
    if (state.sortKey === key) {
      state.sortDir *= -1;
    } else {
      state.sortKey = key;
      state.sortDir = 1;
    }
    render();
  });
});

searchInput.addEventListener("input", () => {
  render();
});

showDuplicates.addEventListener("change", () => {
  render();
});

reloadBtn.addEventListener("click", () => {
  loadData();
});

if (refreshLogsBtn) {
  refreshLogsBtn.addEventListener("click", () => {
    fetchLogs(true);
  });
}

audioEl.addEventListener("ended", () => {
  if (state.activeRow) {
    state.activeRow.classList.remove("active");
    state.activeRow = null;
    currentTitleEl.textContent = "未选择曲目";
  }
  stopVisualization();
});
audioEl.addEventListener("pause", stopVisualization);
audioEl.addEventListener("play", () => {
  startVisualization().catch((error) => {
    console.error(error);
    showToast("浏览器阻止了音频可视化", true);
  });
});

async function loadData() {
  setStatus("正在同步曲目列表…");
  try {
    const res = await fetch("/index.json", { cache: "no-store" });
    if (!res.ok) throw new Error(`请求失败: ${res.status}`);
    const list = await res.json();
    state.records = decorate(list);
    totalCountEl.textContent = state.records.length;
    const duplicateTotal = state.records.filter((item) => item.isDuplicate).length;
    duplicateCountEl.textContent = duplicateTotal;
    setStatus(`更新成功，最后加载 ${new Date().toLocaleTimeString()}`);
    render();
  } catch (error) {
    console.error(error);
    setStatus(`加载失败：${error.message}`);
  }
}

function decorate(list) {
  const titleCount = new Map();
  list.forEach((item) => {
    const title = item.title?.trim() || "";
    titleCount.set(title, (titleCount.get(title) || 0) + 1);
  });

  return list.map((item) => {
    const title = item.title?.trim() || "未命名";
    const match = title.match(/^(\d{1,4})\s+/);
    const index = match ? parseInt(match[1], 10) : null;
    const url = String(item.url || "");
    const formatMatch = url.match(/\.(\w+?)(?:\?.*)?$/);
    const format = formatMatch ? formatMatch[1].toUpperCase() : "N/A";

    return {
      raw: item,
      title,
      index,
      url,
      format,
      isDuplicate: (titleCount.get(title) || 0) > 1,
    };
  });
}

function render() {
  const term = searchInput.value.trim().toLowerCase();
  const allowDuplicates = showDuplicates.checked;

  let records = state.records.filter((item) => {
    if (!allowDuplicates && item.isDuplicate) return false;
    if (!term) return true;
    return item.title.toLowerCase().includes(term) || item.url.toLowerCase().includes(term);
  });

  if (state.sortKey === "title") {
    records.sort((a, b) => a.title.localeCompare(b.title, "zh-Hans-CN") * state.sortDir);
  } else if (state.sortKey === "index") {
    records.sort((a, b) => {
      const av = a.index ?? Number.MAX_SAFE_INTEGER;
      const bv = b.index ?? Number.MAX_SAFE_INTEGER;
      if (av === bv) {
        return a.title.localeCompare(b.title, "zh-Hans-CN");
      }
      return (av - bv) * state.sortDir;
    });
  }

  filteredCountEl.textContent = records.length;
  tableBody.textContent = "";

  records.forEach((item) => {
    const fragment = rowTemplate.content.cloneNode(true);
    const row = fragment.querySelector("tr");
    row.dataset.url = item.url;

    const duplicateBadge = fragment.querySelector(".badge");
    if (!item.isDuplicate) {
      duplicateBadge.classList.add("hidden");
    }

    fragment.querySelector(".label").textContent = item.title;
    fragment.querySelector(".format").textContent = item.format;
    fragment.querySelector(".index").textContent = item.index ?? "-";

    const playBtn = fragment.querySelector(".play");
    const copyBtn = fragment.querySelector(".copy");
    const downloadLink = fragment.querySelector(".download");

    playBtn.addEventListener("click", () => togglePlay(row, item));
    row.addEventListener("dblclick", () => togglePlay(row, item));

    copyBtn.addEventListener("click", async () => {
      try {
        await navigator.clipboard.writeText(item.url);
        showToast("已复制链接");
      } catch (error) {
        console.error(error);
        showToast("复制失败，请检查浏览器权限", true);
      }
    });

    downloadLink.href = buildDownloadUrl(item.url);

    tableBody.appendChild(fragment);
  });
}

function buildDownloadUrl(url) {
  if (!url) return "#";
  return url.includes("?") ? `${url}&download=1` : `${url}?download=1`;
}

function togglePlay(row, item) {
  if (!item?.url) return;

  const isActive = state.activeRow === row && !audioEl.paused;
  if (isActive) {
    audioEl.pause();
    row.classList.remove("active");
    state.activeRow = null;
    currentTitleEl.textContent = "未选择曲目";
    return;
  }

  if (state.activeRow && state.activeRow !== row) {
    state.activeRow.classList.remove("active");
  }

  state.activeRow = row;
  row.classList.add("active");
  audioEl.src = item.url;
  audioEl.play().then(() => {
    currentTitleEl.textContent = item.title;
  }).catch((error) => {
    console.error(error);
    showToast("无法开始播放，请检查浏览器策略", true);
    row.classList.remove("active");
    state.activeRow = null;
    currentTitleEl.textContent = "未选择曲目";
  });
}

function setStatus(message) {
  statusEl.textContent = message;
  statusEl.style.color = "";
}

let toastTimer = null;
function showToast(message, isError = false) {
  clearTimeout(toastTimer);
  statusEl.textContent = message;
  statusEl.style.color = isError ? "var(--danger)" : "";
  toastTimer = setTimeout(() => {
    statusEl.textContent = "";
    statusEl.style.color = "";
  }, 3000);
}

async function fetchLogs(isManual = false) {
  if (!logMetaEl) return;
  logMetaEl.textContent = isManual ? "手动刷新…" : "同步日志…";
  try {
    const res = await fetch("/admin/api/logs?limit=160", { cache: "no-store" });
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const data = await res.json();
    renderLogSummary(data);
    const stamp = data.generated_at
      ? new Date(data.generated_at).toLocaleTimeString()
      : new Date().toLocaleTimeString();
    logMetaEl.textContent = `最近 ${data.entries.length} 条 · 更新 ${stamp}`;
  } catch (error) {
    console.error(error);
    logMetaEl.textContent = `日志加载失败：${error.message}`;
  }
}

function renderLogSummary(data) {
  if (ipSummaryBody) {
    ipSummaryBody.textContent = "";
    const rows = (data.per_ip || []).slice(0, 12);
    rows.forEach((item) => {
      const tr = document.createElement("tr");
      const lastAgo = formatAgo(item.last_ts);
      const cells = [
        item.ip || "-",
        item.user || "访客",
        `${item.hits}`,
        `${item.downloads}`,
        `${lastAgo} · ${item.last_uri || ""}`,
      ];
      cells.forEach((value, idx) => {
        const td = document.createElement("td");
        td.textContent = value;
        if (idx === 4) {
          td.style.maxWidth = "280px";
          td.style.whiteSpace = "nowrap";
          td.style.overflow = "hidden";
          td.style.textOverflow = "ellipsis";
        }
        tr.appendChild(td);
      });
      ipSummaryBody.appendChild(tr);
    });
    if (!rows.length) {
      const tr = document.createElement("tr");
      const td = document.createElement("td");
      td.colSpan = 5;
      td.textContent = "暂无访问记录";
      td.style.textAlign = "center";
      td.style.color = "rgba(168, 190, 224, 0.55)";
      tr.appendChild(td);
      ipSummaryBody.appendChild(tr);
    }
  }

  if (logStreamEl) {
    logStreamEl.textContent = "";
    const entries = (data.entries || []).slice(-40).reverse();
    entries.forEach((entry) => {
      const li = document.createElement("li");
      const topLine = document.createElement("div");
      topLine.textContent = `${entry.method || ""} ${entry.status || ""} · ${entry.ip || "-"}`;
      li.appendChild(topLine);

      const uriLine = document.createElement("div");
      uriLine.textContent = entry.uri || "/";
      uriLine.style.color = "var(--fg)";
      li.appendChild(uriLine);

      const metaLine = document.createElement("div");
      const duration = entry.duration_ms != null ? `${entry.duration_ms}ms` : "";
      metaLine.textContent = `${formatAgo(entry.ts)} · ${duration} · ${entry.user || "访客"}`;
      metaLine.style.color = "rgba(168, 190, 224, 0.6)";
      li.appendChild(metaLine);

      logStreamEl.appendChild(li);
    });
    if (!entries.length) {
      const li = document.createElement("li");
      li.textContent = "暂无日志事件";
      li.style.textAlign = "center";
      li.style.color = "rgba(168, 190, 224, 0.55)";
      logStreamEl.appendChild(li);
    }
  }
}

function formatAgo(ts) {
  if (!ts) return "未知时间";
  const diff = Date.now() - ts * 1000;
  const abs = Math.max(0, diff);
  if (abs < 60000) {
    return "刚刚";
  }
  if (abs < 3600000) {
    const mins = Math.round(abs / 60000);
    return `${mins} 分钟前`;
  }
  if (abs < 86400000) {
    const hours = Math.round(abs / 3600000);
    return `${hours} 小时前`;
  }
  const days = Math.round(abs / 86400000);
  return `${days} 天前`;
}

function scheduleLogRefresh() {
  if (!logMetaEl) return;
  if (logTimer) {
    clearInterval(logTimer);
  }
  fetchLogs(false);
  logTimer = setInterval(() => {
    if (document.hidden) return;
    fetchLogs(false);
  }, 15000);
}

document.addEventListener("visibilitychange", () => {
  if (!document.hidden) {
    fetchLogs(false);
  }
});

function resizeVisualizer() {
  if (!visualizerCanvas || !visualizerCtx) return;
  const dpr = window.devicePixelRatio || 1;
  const rect = visualizerCanvas.getBoundingClientRect();
  visualizerCanvas.width = Math.round(rect.width * dpr);
  visualizerCanvas.height = Math.round(rect.height * dpr);
}

function ensureAudioGraph() {
  if (audioContext) return;
  const AudioCtx = window.AudioContext || window.webkitAudioContext;
  if (!AudioCtx) {
    throw new Error("浏览器不支持 Web Audio API");
  }
  audioContext = new AudioCtx();
  analyserNode = audioContext.createAnalyser();
  analyserNode.fftSize = 1024;
  const bufferLength = analyserNode.frequencyBinCount;
  analyserData = new Uint8Array(bufferLength);
  sourceNode = audioContext.createMediaElementSource(audioEl);
  sourceNode.connect(analyserNode);
  analyserNode.connect(audioContext.destination);
}

async function startVisualization() {
  if (!visualizerCanvas || !visualizerCtx) return;
  ensureAudioGraph();
  if (!audioContext) return;
  if (audioContext.state === "suspended") {
    await audioContext.resume();
  }
  if (visualizerFrame) return;
  resizeVisualizer();
  renderVisualizer();
}

function stopVisualization() {
  if (visualizerFrame) {
    cancelAnimationFrame(visualizerFrame);
    visualizerFrame = null;
  }
  if (!visualizerCtx || !visualizerCanvas) return;
  visualizerCtx.fillStyle = "rgba(6, 10, 28, 0.72)";
  visualizerCtx.fillRect(0, 0, visualizerCanvas.width, visualizerCanvas.height);
}

function renderVisualizer() {
  if (!analyserNode || !visualizerCtx) return;
  analyserNode.getByteFrequencyData(analyserData);
  const { width, height } = visualizerCanvas;
  visualizerCtx.clearRect(0, 0, width, height);

  const barCount = 72;
  const step = Math.max(1, Math.floor(analyserData.length / barCount));
  const barWidth = width / barCount;
  const baseY = height * 0.92;

  const gradient = visualizerCtx.createLinearGradient(0, 0, 0, height);
  gradient.addColorStop(0, "rgba(83, 248, 255, 0.95)");
  gradient.addColorStop(0.45, "rgba(159, 75, 255, 0.85)");
  gradient.addColorStop(1, "rgba(13, 20, 54, 0.4)");
  visualizerCtx.fillStyle = gradient;

  for (let i = 0; i < barCount; i += 1) {
    const dataIndex = i * step;
    const magnitude = analyserData[dataIndex] / 255;
    const barHeight = Math.max(height * 0.05, magnitude * height * 0.9);
    const x = i * barWidth + barWidth * 0.12;
    const y = baseY - barHeight;
    const radius = Math.min(barWidth * 0.4, 12);

    visualizerCtx.beginPath();
    visualizerCtx.moveTo(x, baseY);
    visualizerCtx.lineTo(x, y + radius);
    visualizerCtx.quadraticCurveTo(x, y, x + radius, y);
    visualizerCtx.lineTo(x + barWidth - radius * 2, y);
    visualizerCtx.quadraticCurveTo(x + barWidth, y, x + barWidth, y + radius);
    visualizerCtx.lineTo(x + barWidth, baseY);
    visualizerCtx.closePath();
    visualizerCtx.fill();
  }

  visualizerFrame = requestAnimationFrame(renderVisualizer);
}

loadData();
scheduleLogRefresh();
