import api from './http';
import type { User } from '@/types/models';

interface AuthResponse {
  accessToken: string;
  refreshToken: string;
  username: string;
  role: string;
}

export async function login(payload: { username: string; password: string }) {
  const response = await api.post<{ code: number; message: string; data: AuthResponse }>('/auth/login', payload);
  return response.data.data;
}

export async function register(payload: { username: string; email?: string; password: string }) {
  const response = await api.post<{ code: number; message: string; data: AuthResponse }>('/auth/register', payload);
  return response.data.data;
}

export async function fetchProfile() {
  const { data } = await api.post<{ code?: number; data: User }>('/auth/profile');
  // profile is wrapped
  return (data as any).data ?? data;
}
