import api from './http';
import type { AlertRule, SeasonResponse } from '@/types/models';
import type { ApiResponse } from '@/types/api';

export async function listAlertRules(enabled?: boolean) {
  const { data } = await api.get<ApiResponse<AlertRule[]> | AlertRule[]>('/alert-rules', {
    params: { enabled }
  });
  // 兼容后端直接返回数组或包裹在 data 中的两种形式
  return Array.isArray((data as any).data) ? (data as ApiResponse<AlertRule[]>).data : (data as AlertRule[]);
}

export async function getAlertRule(id: number) {
  const { data } = await api.get<ApiResponse<AlertRule> | AlertRule>(`/alert-rules/${id}`);
  return (data as ApiResponse<AlertRule>).data ?? (data as AlertRule);
}

export async function createAlertRule(payload: AlertRule) {
  const { data } = await api.post<ApiResponse<AlertRule> | AlertRule>('/alert-rules', payload);
  return (data as ApiResponse<AlertRule>).data ?? (data as AlertRule);
}

export async function updateAlertRule(id: number, payload: AlertRule) {
  const { data } = await api.put<ApiResponse<AlertRule> | AlertRule>(`/alert-rules/${id}`, payload);
  return (data as ApiResponse<AlertRule>).data ?? (data as AlertRule);
}

export async function toggleAlertRule(id: number, enabled: boolean) {
  await api.patch(`/alert-rules/${id}/enabled`, null, { params: { enabled } });
}

export async function deleteAlertRule(id: number) {
  await api.delete(`/alert-rules/${id}`);
}

export async function getCurrentSeason() {
  const { data } = await api.get<ApiResponse<SeasonResponse> | SeasonResponse>('/alert-rules/current-season');
  return (data as ApiResponse<SeasonResponse>).data ?? (data as SeasonResponse);
}

export async function listSystemTemplates(season?: string) {
  const { data } = await api.get<ApiResponse<AlertRule[]> | AlertRule[]>('/alert-rules/templates', {
    params: { season }
  });
  return Array.isArray((data as any).data) ? (data as ApiResponse<AlertRule[]>).data : (data as AlertRule[]);
}

export async function listUserRules(deviceId?: string) {
  const { data } = await api.get<ApiResponse<AlertRule[]> | AlertRule[]>('/alert-rules/user-rules', {
    params: { deviceId }
  });
  return Array.isArray((data as any).data) ? (data as ApiResponse<AlertRule[]>).data : (data as AlertRule[]);
}
