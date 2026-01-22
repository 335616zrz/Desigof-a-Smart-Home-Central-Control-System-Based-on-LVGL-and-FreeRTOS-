import { createApp } from 'vue';
import { createPinia } from 'pinia';
import App from './App.vue';
import router from './router';
import './styles/base.css';
import './styles/theme.css';
import './styles/responsive.css';
import 'element-plus/theme-chalk/dark/css-vars.css';
import { useAppStore } from './stores/app';
import { useTelemetryStream } from './stores/telemetryStream';

const app = createApp(App);
const pinia = createPinia();
app.use(pinia);
const appStore = useAppStore();
appStore.initTheme();
useTelemetryStream().hydrate();
// 尝试恢复登录态并刷新个人信息
import('./stores/auth').then(({ useAuthStore }) => {
  const auth = useAuthStore();
  if (auth.token && !auth.user) {
    auth.loadProfile();
  }
});
app.use(router);
app.mount('#app');
