import api from './http';
import type { ApiResponse } from '@/types/api';
import type { AlertItem, Paginated } from '@/types/models';

export async function listAlerts(params?: { page?: number; pageSize?: number; status?: string; level?: string }) {
  const { data } = await api.get<ApiResponse<Paginated<AlertItem>> | Paginated<AlertItem>>('/alerts', { params });
  // 兼容后端直接返回分页对象或包裹在 data 中的两种形式
  return (data as ApiResponse<Paginated<AlertItem>>).data ?? (data as Paginated<AlertItem>);
}

export async function getAlert(id: number) {
  const { data } = await api.get<ApiResponse<AlertItem> | AlertItem>(`/alerts/${id}`);
  return (data as ApiResponse<AlertItem>).data ?? (data as AlertItem);
}

export async function acknowledgeAlert(id: number) {
  await api.post(`/alerts/${id}/ack`);
}

export async function closeAlert(id: number) {
  await api.post(`/alerts/${id}/close`);
}

export async function exportAlerts(params?: { status?: string; level?: string; deviceId?: string; from?: string; to?: string }) {
  const resp = await api.get('/alerts/export', {
    params,
    responseType: 'blob'
  });

  const contentType = resp.headers?.['content-type'] || 'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet';
  const blob = new Blob([resp.data], { type: contentType });

  const disposition = resp.headers?.['content-disposition'] as string | undefined;
  const filename = parseFilenameFromDisposition(disposition) || 'alerts.xlsx';

  const url = window.URL.createObjectURL(blob);
  const link = document.createElement('a');
  link.href = url;
  link.download = filename;
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
  window.URL.revokeObjectURL(url);
}

function parseFilenameFromDisposition(disposition?: string): string | null {
  if (!disposition) return null;

  // RFC 5987: filename*=UTF-8''...
  const utf8Match = disposition.match(/filename\*\s*=\s*UTF-8''([^;]+)/i);
  if (utf8Match?.[1]) {
    try {
      return decodeURIComponent(utf8Match[1]);
    } catch {
      return utf8Match[1];
    }
  }

  const normalMatch = disposition.match(/filename\s*=\s*\"?([^\";]+)\"?/i);
  return normalMatch?.[1] || null;
}
