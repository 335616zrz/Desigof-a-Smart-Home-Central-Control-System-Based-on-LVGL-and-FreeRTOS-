import { defineStore } from 'pinia';
import api from '@/api/http';

type ThemeMode = 'light' | 'dark' | 'auto';

const isThemeMode = (value: string | null): value is ThemeMode =>
  value === 'light' || value === 'dark' || value === 'auto';

const resolveStoredTheme = (): ThemeMode => {
  const current = localStorage.getItem('iot_theme');
  const legacy = localStorage.getItem('theme-mode');
  const picked = current || legacy;

  const theme = isThemeMode(picked) ? picked : 'auto';

  // Migrate legacy storage so login/dashboard switches read the same key
  if (!current && isThemeMode(legacy)) {
    localStorage.setItem('iot_theme', legacy);
  }

  return theme;
};

const storedTheme = resolveStoredTheme();
const storedApiBase = localStorage.getItem('iot_api_base') || '';
const storedMqtt = localStorage.getItem('iot_mqtt_ws') || '';

export const useAppStore = defineStore('app', {
  state: () => ({
    collapsed: false,
    theme: storedTheme as ThemeMode,
    apiBase: storedApiBase || (import.meta.env.VITE_API_BASE_URL || '/api'),
    mqttWsUrl: storedMqtt || (import.meta.env.VITE_MQTT_WS_URL || 'ws://localhost:8083/mqtt')
  }),
  actions: {
    toggleSidebar() {
      this.collapsed = !this.collapsed;
    },
    resolveEffectiveTheme(mode: ThemeMode) {
      if (mode === 'auto') {
        const hour = new Date().getHours();
        return hour >= 8 && hour < 22 ? 'light' : 'dark';
      }
      return mode;
    },
    applyTheme(mode: ThemeMode) {
      const effective = this.resolveEffectiveTheme(mode);
      document.body.classList.toggle('dark', effective === 'dark');
      document.documentElement.classList.toggle('theme-dark', effective === 'dark');
      document.documentElement.classList.toggle('theme-light', effective === 'light');
    },
    setTheme(theme: ThemeMode) {
      this.theme = theme;
      localStorage.setItem('iot_theme', theme);
      this.applyTheme(theme);
    },
    setApiBase(base: string) {
      this.apiBase = base;
      localStorage.setItem('iot_api_base', base);
      api.defaults.baseURL = base;
    },
    setMqttWsUrl(url: string) {
      this.mqttWsUrl = url;
      localStorage.setItem('iot_mqtt_ws', url);
    },
    initTheme() {
      this.applyTheme(this.theme);
    }
  }
});
