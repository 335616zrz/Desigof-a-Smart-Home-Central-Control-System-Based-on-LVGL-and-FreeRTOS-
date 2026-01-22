import axios from 'axios';
import { useAuthStore } from '@/stores/auth';

const storedBase = localStorage.getItem('iot_api_base');
const api = axios.create({
  baseURL: storedBase || import.meta.env.VITE_API_BASE_URL || '/api',
  timeout: 15000
});

api.interceptors.request.use(config => {
  const auth = useAuthStore();
  if (auth.token) {
    config.headers = config.headers || {};
    config.headers.Authorization = `Bearer ${auth.token}`;
  }
  return config;
});

api.interceptors.response.use(
  response => response,
  async error => {
    const auth = useAuthStore();
    const status = error?.response?.status;

    // 403 通常表示权限不足（非 token 失效）。不要在这里清理 token，否则会导致误登出。

    // Handle 401 Unauthorized with refresh token
    if (status === 401 && auth.refreshToken) {
      try {
        const { data } = await api.post('/auth/refresh', {
          refreshToken: auth.refreshToken
        });
        const token = data.data?.accessToken ?? data.accessToken;
        const refresh = data.data?.refreshToken ?? data.refreshToken;
        auth.setTokens(token, refresh);
        const original = error.config;
        original.headers = original.headers || {};
        original.headers.Authorization = `Bearer ${auth.token}`;
        return api(original);
      } catch (refreshErr) {
        auth.logout();
      }
    }
    return Promise.reject(error);
  }
);

export default api;
