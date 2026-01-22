import { defineStore } from 'pinia';
import { login as apiLogin, register as apiRegister, fetchProfile } from '@/api/auth';
import type { User } from '@/types/models';
import { decodeJwt, isTokenExpired } from '@/utils/jwt';

interface AuthState {
  token: string | null;
  refreshToken: string | null;
  user: User | null;
  loading: boolean;
}

const TOKEN_KEY = 'iot_token';
const REFRESH_KEY = 'iot_refresh';

function getUserFromToken(token: string | null): User | null {
  if (!token) return null;

  // Check if token is expired
  if (isTokenExpired(token)) {
    localStorage.removeItem(TOKEN_KEY);
    localStorage.removeItem(REFRESH_KEY);
    return null;
  }

  const payload = decodeJwt(token);
  if (!payload) return null;

  const username = (payload as any).sub || (payload as any).username || (payload as any).user_name;
  const role = (payload as any).role || (payload as any).roles || (payload as any).authorities;

  if (!username) return null;

  return {
    id: 0,
    username,
    role,
    email: '',
    createdAt: ''
  } as User;
}

export const useAuthStore = defineStore('auth', {
  state: (): AuthState => {
    const token = localStorage.getItem(TOKEN_KEY);
    return {
      token,
      refreshToken: localStorage.getItem(REFRESH_KEY),
      user: getUserFromToken(token),
      loading: false
    };
  },
  getters: {
    isAuthenticated: state => {
      if (!state.token) return false;
      // Check if token is expired
      if (isTokenExpired(state.token)) {
        return false;
      }
      return true;
    }
  },
  actions: {
    setTokens(accessToken: string, refresh?: string) {
      this.token = accessToken;
      this.refreshToken = refresh || null;
      this.user = getUserFromToken(accessToken);
      localStorage.setItem(TOKEN_KEY, accessToken);
      if (refresh) localStorage.setItem(REFRESH_KEY, refresh);
    },
    clearTokens() {
      this.token = null;
      this.refreshToken = null;
      localStorage.removeItem(TOKEN_KEY);
      localStorage.removeItem(REFRESH_KEY);
    },
    async login(username: string, password: string) {
      this.loading = true;
      try {
        const result = await apiLogin({ username, password });
        const access = (result as any).accessToken || (result as any).token;
        this.setTokens(access, (result as any).refreshToken);
        // 优先使用后端 profile，避免 token claim 缺少字段
        try {
          this.user = await fetchProfile();
        } catch {
          this.user = { id: 0, username: result.username, role: result.role as any } as User;
        }
      } finally {
        this.loading = false;
      }
    },
    async register(username: string, email: string, password: string) {
      this.loading = true;
      try {
        const result = await apiRegister({ username, email, password });
        this.user = { id: 0, username: result.username, role: result.role as any } as User;
        const access = (result as any).accessToken || (result as any).token;
        this.setTokens(access, (result as any).refreshToken);
      } finally {
        this.loading = false;
      }
    },
    async loadProfile() {
      if (!this.token) return;
      try {
        this.user = await fetchProfile();
      } catch (error) {
        console.error('Failed to load profile', error);
      }
    },
    logout() {
      this.clearTokens();
      this.user = null;
    }
  }
});
