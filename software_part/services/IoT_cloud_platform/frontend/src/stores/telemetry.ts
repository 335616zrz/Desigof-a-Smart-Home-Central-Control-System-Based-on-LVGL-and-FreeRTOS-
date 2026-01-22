import { defineStore } from 'pinia';
import { fetchHistory } from '@/api/telemetry';
import type { TelemetryRecord, Paginated } from '@/types/models';
import dayjs from 'dayjs';

interface TelemetryState {
  history: TelemetryRecord[];
  total: number;
  loading: boolean;
}

export const useTelemetryStore = defineStore('telemetry', {
  state: (): TelemetryState => ({
    history: [],
    total: 0,
    loading: false
  }),
  actions: {
    async loadHistory(params: { deviceId?: string; from?: string; to?: string; metric?: string; page?: number; pageSize?: number }) {
      this.loading = true;
      try {
        const result = await fetchHistory(params);
        const { items, total } = result;

        // Parse payload JSON and extract sensor data
        this.history = items.map(item => {
          try {
            // Parse payload if it's a string
            const parsed = typeof item.payload === 'string' ? JSON.parse(item.payload) : item.payload || {};

            // Extract sensor data from payload.data or top level
            const sensorData = parsed.data || parsed;

            // Use reportedAt as primary timestamp source (database timestamp)
            // This ensures accurate time regardless of device/payload timestamp
            const timestamp = item.reportedAt ? dayjs(item.reportedAt).valueOf() : Date.now();

            return {
              id: item.id,
              deviceId: item.deviceId,
              payload: item.payload,
              reportedAt: item.reportedAt,
              timestamp,
              temperature: sensorData.temperature,
              humidity: sensorData.humidity,
              pressure: sensorData.pressure,
              light: sensorData.light
            } as TelemetryRecord;
          } catch (err) {
            console.warn('Failed to parse telemetry payload:', err, item);
            // Fallback: try to use reportedAt as timestamp
            return {
              ...item,
              timestamp: item.reportedAt ? dayjs(item.reportedAt).valueOf() : Date.now()
            } as TelemetryRecord;
          }
        });
        this.total = total;
      } catch (error) {
        console.error('Failed to load telemetry history:', error);
        this.history = [];
        this.total = 0;
      } finally {
        this.loading = false;
      }
    }
  }
});
