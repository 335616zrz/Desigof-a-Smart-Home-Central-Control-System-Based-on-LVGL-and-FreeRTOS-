import api from './http';
import type { ApiResponse } from '@/types/api';
import type { Paginated, Device, DeviceCredential } from '@/types/models';

export interface DeviceShadowView {
  deviceId: string;
  nightLightOn: boolean;
  nightLightSource?: 'reported' | 'desired' | 'default' | string;
  nightLightPending: boolean;
  alarmActive: boolean;
  alarmSource?: 'reported' | 'desired' | 'default' | string;
  alarmPending: boolean;
  desiredNightLightOn?: boolean | null;
  reportedNightLightOn?: boolean | null;
  desiredAlarmActive?: boolean | null;
  reportedAlarmActive?: boolean | null;
  desiredUpdatedAt?: string | null;
  reportedUpdatedAt?: string | null;
}

export async function listDevices(params?: { page?: number; pageSize?: number; keyword?: string }) {
  const { data } = await api.get<ApiResponse<Paginated<Device>>>('/devices', { params });
  return (data as any).data ?? data;
}

export async function getDevice(id: string) {
  const { data } = await api.get<ApiResponse<Device>>(`/devices/${id}`);
  return (data as any).data ?? data;
}

export async function createDevice(payload: Partial<Device>) {
  const { data } = await api.post<ApiResponse<Device>>('/devices', payload);
  return (data as any).data ?? data;
}

export async function updateDevice(id: string, payload: Partial<Device>) {
  const { data } = await api.put<ApiResponse<Device>>(`/devices/${id}`, payload);
  return (data as any).data ?? data;
}

export async function deleteDevice(id: string) {
  await api.delete(`/devices/${id}`);
}

export async function provisionDevice(id: string) {
  const { data } = await api.post<ApiResponse<DeviceCredential>>(`/devices/${id}/provision`);
  return data.data;
}

export async function setNightLight(id: string, on: boolean) {
  const { data } = await api.post<ApiResponse<{ deviceId: string; on: boolean }>>(
    `/devices/${id}/commands/night-light`,
    { on: Boolean(on) }
  );
  return (data as any).data ?? data;
}

export async function getDeviceShadow(id: string) {
  const { data } = await api.get<ApiResponse<DeviceShadowView>>(`/devices/${id}/shadow`);
  return (data as any).data ?? data;
}
