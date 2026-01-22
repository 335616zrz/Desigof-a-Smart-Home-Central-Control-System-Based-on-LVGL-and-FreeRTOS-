import { fetchHistory, fetchRealtime } from '../api/telemetry.js'
import { defineStore } from './pinia.js'

function unwrap(resp) {
  return resp?.data ?? resp ?? null
}

function toNumberOrNull(v) {
  if (v === null || v === undefined) return null
  const n = Number(v)
  return Number.isFinite(n) ? n : null
}

// Parse a telemetry record payload into structured fields.
// Supports payload shapes like:
//  - '{"temperature":25,"humidity":50}'
//  - '{"data":{"temperature":25,"humidity":50}}'
export function parseTelemetryRecord(record) {
  if (!record) return null

  let payload = record.payload
  if (typeof payload === 'string') {
    try {
      payload = JSON.parse(payload)
    } catch {
      payload = {}
    }
  }
  const data = payload?.data || payload || {}

  const ts = new Date(record.reportedAt || record.timestamp || 0).getTime()

  return {
    ...record,
    temperature: toNumberOrNull(data?.temperature),
    humidity: toNumberOrNull(data?.humidity),
    pressure: toNumberOrNull(data?.pressure),
    timestamp: Number.isFinite(ts) ? ts : 0,
  }
}

export const useTelemetryStore = defineStore('telemetry', {
  state: () => ({
    latest: [],
    history: [],
    // Used to short-circuit updates when polling returns the same newest record.
    // This reduces reactive churn + devtools memory usage.
    _historyHeadKey: '',
    // Monotonic seq used to ignore stale async responses (e.g. live polling racing with history switch).
    _historyReqSeq: 0,
    total: 0,
    page: 1,
    pageSize: 100,
    loading: false,
  }),
  getters: {
    parsedHistory() {
      return (this.history || [])
        .map(parseTelemetryRecord)
        .filter((r) => r && r.deviceId)
    },

    // Keep the latest record per device (dedupe).
    latestByDevice() {
      const map = new Map()
      for (const r of this.parsedHistory || []) {
        const prev = map.get(r.deviceId)
        if (!prev || r.timestamp > prev.timestamp) map.set(r.deviceId, r)
      }
      return Array.from(map.values()).sort((a, b) => b.timestamp - a.timestamp)
    },

    // Chart data (last 50 points, sorted ascending).
    temperatureChartData() {
      // Backend returns records sorted DESC (latest first). Sort by timestamp first, then take the newest 50.
      const list = (this.parsedHistory || [])
        .filter((r) => r && r.temperature !== null && (r.timestamp || 0) > 0)
        .sort((a, b) => a.timestamp - b.timestamp)
      return list.slice(-50)
    },

    humidityChartData() {
      const list = (this.parsedHistory || [])
        .filter((r) => r && r.humidity !== null && (r.timestamp || 0) > 0)
        .sort((a, b) => a.timestamp - b.timestamp)
      return list.slice(-50)
    },
  },
  actions: {
    async fetchRealtime(deviceId, limit = 50) {
      const resp = await fetchRealtime(deviceId, limit)
      const data = unwrap(resp)
      this.latest = data || []
      return this.latest
    },

    async fetchHistory(params) {
      const reqSeq = ++this._historyReqSeq
      this.loading = true
      try {
        const resp = await fetchHistory(params)
        const data = unwrap(resp)

        // If a newer request has started since this one, ignore this response.
        // This prevents "history -> instantly becomes live(10min)" caused by out-of-order responses.
        if (reqSeq !== this._historyReqSeq) return data

        const items = data?.items || []
        const head = items[0] || null
        const headKey = head ? `${head.id || ''}|${head.reportedAt || head.timestamp || ''}` : ''
        if (
          headKey &&
          headKey === this._historyHeadKey &&
          Array.isArray(this.history) &&
          this.history.length === items.length
        ) {
          // Keep existing arrays to avoid triggering large rerenders when nothing changed.
          this.total = data?.total || this.total
          this.page = data?.page || this.page
          this.pageSize = data?.pageSize || data?.page_size || this.pageSize
          return data
        }

        this.history = items
        this._historyHeadKey = headKey
        this.total = data?.total || 0
        this.page = data?.page || 1
        this.pageSize = data?.pageSize || data?.page_size || 100
        return data
      } finally {
        // Only clear loading state for the latest request.
        if (reqSeq === this._historyReqSeq) this.loading = false
      }
    },
  },
})
