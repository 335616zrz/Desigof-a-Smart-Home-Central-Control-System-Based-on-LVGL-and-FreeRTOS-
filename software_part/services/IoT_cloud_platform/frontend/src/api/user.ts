import api from './http';
import type { ApiResponse } from '@/types/api';
import type { Paginated, User } from '@/types/models';

export async function listUsers(params?: { page?: number; pageSize?: number; keyword?: string }) {
  const { data } = await api.get<Paginated<User>>('/users', { params });
  return data;
}

export async function createUser(payload: { username: string; password: string; email?: string; role: string }) {
  const { data } = await api.post<User>('/users', payload);
  return data;
}

export async function deleteUser(id: string) {
  await api.delete(`/users/${id}`);
}

export async function batchCreate(users: Array<{ username: string; password: string; email?: string; role: string }>) {
  const { data } = await api.post<User[]>('/users/batch-create', users);
  return data;
}

export async function batchDelete(ids: number[]) {
  await api.post('/users/batch-delete', ids);
}

export async function upgradeUser(id: number) {
  const { data } = await api.post<User>(`/users/${id}/upgrade`);
  return data;
}

export async function downgradeUser(id: number) {
  const { data } = await api.post<User>(`/users/${id}/downgrade`);
  return data;
}
