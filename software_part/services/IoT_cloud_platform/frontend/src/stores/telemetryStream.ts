import { defineStore } from 'pinia';
import type { TelemetryRecord } from '@/types/models';

interface StreamState {
  connected: boolean;
  points: TelemetryRecord[];
  max: number;
}

const STORAGE_KEY = 'iot.telemetryStream.v1';
const MAX_CACHE_AGE_MS = 24 * 60 * 60 * 1000; // 24h

export const useTelemetryStream = defineStore('telemetry-stream', {
  state: (): StreamState => ({
    connected: false,
    points: [],
    max: 1200
  }),
  actions: {
    hydrate() {
      if (typeof window === 'undefined') return;
      try {
        const raw = window.localStorage.getItem(STORAGE_KEY);
        if (!raw) return;
        const parsed = JSON.parse(raw) as unknown;
        if (!Array.isArray(parsed)) return;

        const now = Date.now();
        const hydrated: TelemetryRecord[] = parsed
          .filter((item: any) => item && typeof item === 'object')
          .map((item: any) => ({
            deviceId: String(item.deviceId ?? ''),
            timestamp: Number(item.timestamp ?? 0),
            temperature: typeof item.temperature === 'number' ? item.temperature : undefined,
            humidity: typeof item.humidity === 'number' ? item.humidity : undefined,
            pressure: typeof item.pressure === 'number' ? item.pressure : undefined,
            light: typeof item.light === 'number' ? item.light : undefined
          }))
          .filter(p => p.deviceId && Number.isFinite(p.timestamp) && p.timestamp > 0)
          .filter(p => p.timestamp >= now - MAX_CACHE_AGE_MS)
          .sort((a, b) => a.timestamp - b.timestamp);

        this.points = hydrated.slice(-this.max);
      } catch {
        // ignore corrupted cache
      }
    },
    persist() {
      if (typeof window === 'undefined') return;
      try {
        const payload = JSON.stringify(this.points.slice(-this.max));
        window.localStorage.setItem(STORAGE_KEY, payload);
      } catch {
        // ignore quota/circular errors
      }
    },
    setConnected(value: boolean) {
      this.connected = value;
    },
    push(deviceId: string, payload: Record<string, unknown>) {
      const toFiniteNumber = (value: unknown) => {
        const n = typeof value === 'number' ? value : Number(value);
        return Number.isFinite(n) ? n : undefined;
      };
      const record: TelemetryRecord = {
        deviceId,
        timestamp: Date.now(),
        temperature: toFiniteNumber(payload.temperature),
        humidity: toFiniteNumber(payload.humidity),
        pressure: toFiniteNumber(payload.pressure),
        light: toFiniteNumber(payload.light)
      };
      this.points.push(record);
      if (this.points.length > this.max) this.points.shift();
      this.persist();
    }
  }
});
