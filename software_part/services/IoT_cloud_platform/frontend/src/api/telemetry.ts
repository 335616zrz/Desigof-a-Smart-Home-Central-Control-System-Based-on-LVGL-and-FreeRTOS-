import api from './http';
import type { ApiResponse } from '@/types/api';
import type { Paginated, TelemetryRecord } from '@/types/models';

export async function fetchRealtime(deviceId: string, limit: number = 50) {
  const { data } = await api.get<ApiResponse<TelemetryRecord[]>>(`/telemetry/latest`, {
    params: { deviceId, limit }
  });
  return data.data;
}

export async function fetchHistory(params: {
  deviceId?: string;
  from?: string;
  to?: string;
  metric?: string;
  page?: number;
  pageSize?: number;
}) {
  const { data } = await api.get<ApiResponse<Paginated<TelemetryRecord>> | Paginated<TelemetryRecord>>('/telemetry', { params });
  const resp: any = (data as any).data ?? data;
  const items = resp.items ?? [];
  return {
    items,
    total: resp.total ?? items.length
  } as Paginated<TelemetryRecord>;
}
