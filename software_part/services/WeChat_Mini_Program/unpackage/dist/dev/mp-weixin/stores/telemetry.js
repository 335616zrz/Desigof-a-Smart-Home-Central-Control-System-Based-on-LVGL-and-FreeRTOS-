"use strict";
const api_telemetry = require("../api/telemetry.js");
const stores_pinia = require("./pinia.js");
function unwrap(resp) {
  return (resp == null ? void 0 : resp.data) ?? resp ?? null;
}
function toNumberOrNull(v) {
  if (v === null || v === void 0)
    return null;
  const n = Number(v);
  return Number.isFinite(n) ? n : null;
}
function parseTelemetryRecord(record) {
  if (!record)
    return null;
  let payload = record.payload;
  if (typeof payload === "string") {
    try {
      payload = JSON.parse(payload);
    } catch {
      payload = {};
    }
  }
  const data = (payload == null ? void 0 : payload.data) || payload || {};
  const ts = new Date(record.reportedAt || record.timestamp || 0).getTime();
  return {
    ...record,
    temperature: toNumberOrNull(data == null ? void 0 : data.temperature),
    humidity: toNumberOrNull(data == null ? void 0 : data.humidity),
    pressure: toNumberOrNull(data == null ? void 0 : data.pressure),
    timestamp: Number.isFinite(ts) ? ts : 0
  };
}
const useTelemetryStore = stores_pinia.defineStore("telemetry", {
  state: () => ({
    latest: [],
    history: [],
    total: 0,
    page: 1,
    pageSize: 100,
    loading: false
  }),
  getters: {
    parsedHistory() {
      return (this.history || []).map(parseTelemetryRecord).filter((r) => r && r.deviceId);
    },
    // Keep the latest record per device (dedupe).
    latestByDevice() {
      const map = /* @__PURE__ */ new Map();
      for (const r of this.parsedHistory || []) {
        const prev = map.get(r.deviceId);
        if (!prev || r.timestamp > prev.timestamp)
          map.set(r.deviceId, r);
      }
      return Array.from(map.values()).sort((a, b) => b.timestamp - a.timestamp);
    },
    // Chart data (last 50 points, sorted ascending).
    temperatureChartData() {
      return (this.parsedHistory || []).filter((r) => r.temperature !== null).slice(-50).sort((a, b) => a.timestamp - b.timestamp);
    },
    humidityChartData() {
      return (this.parsedHistory || []).filter((r) => r.humidity !== null).slice(-50).sort((a, b) => a.timestamp - b.timestamp);
    }
  },
  actions: {
    async fetchRealtime(deviceId, limit = 50) {
      const resp = await api_telemetry.fetchRealtime(deviceId, limit);
      const data = unwrap(resp);
      this.latest = data || [];
      return this.latest;
    },
    async fetchHistory(params) {
      this.loading = true;
      try {
        const resp = await api_telemetry.fetchHistory(params);
        const data = unwrap(resp);
        this.history = (data == null ? void 0 : data.items) || [];
        this.total = (data == null ? void 0 : data.total) || 0;
        this.page = (data == null ? void 0 : data.page) || 1;
        this.pageSize = (data == null ? void 0 : data.pageSize) || (data == null ? void 0 : data.page_size) || 100;
        return data;
      } finally {
        this.loading = false;
      }
    }
  }
});
exports.useTelemetryStore = useTelemetryStore;
//# sourceMappingURL=../../.sourcemap/mp-weixin/stores/telemetry.js.map
