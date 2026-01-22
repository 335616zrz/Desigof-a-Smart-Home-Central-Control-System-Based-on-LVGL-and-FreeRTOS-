import { createRouter, createWebHistory, type RouteRecordRaw } from 'vue-router';
import { useAuthStore } from '@/stores/auth';

// 懒加载布局组件
const BasicLayout = () => import('@/layouts/BasicLayout.vue');
const AuthLayout = () => import('@/layouts/AuthLayout.vue');

const routes: RouteRecordRaw[] = [
  {
    path: '/login',
    component: AuthLayout,
    children: [
      {
        path: '',
        name: 'login',
        component: () => import('@/views/auth/LoginView.vue'),
        meta: { public: true, title: '登录' }
      }
    ]
  },
  {
    path: '/register',
    component: AuthLayout,
    children: [
      {
        path: '',
        name: 'register',
        component: () => import('@/views/auth/RegisterView.vue'),
        meta: { public: true, title: '注册' }
      }
    ]
  },
  {
    path: '/theme-fusion',
    name: 'theme-fusion',
    component: () => import('@/views/theme/ThemeFusionView.vue'),
    meta: { public: true, title: 'Theme Fusion' }
  },
  {
    path: '/',
    component: BasicLayout,
    children: [
      {
        path: '',
        name: 'dashboard',
        component: () => import('@/views/DashboardView.vue'),
        meta: { title: '仪表盘' }
      },
      {
        path: 'devices',
        name: 'devices',
        component: () => import('@/views/device/DeviceListView.vue'),
        meta: { title: '设备' }
      },
      {
        path: 'devices/:id',
        name: 'device-detail',
        component: () => import('@/views/device/DeviceDetailView.vue'),
        meta: { title: '设备详情' }
      },
  {
    path: 'alerts',
    name: 'alerts',
    component: () => import('@/views/alert/AlertListView.vue'),
    meta: { title: '告警' }
  },
  {
    path: 'alerts/:id',
    name: 'alert-detail',
    component: () => import('@/views/alert/AlertDetailView.vue'),
    meta: { title: '告警详情' }
  },
  {
    path: 'alert-rules',
    name: 'alert-rules',
    redirect: { name: 'alerts', query: { tab: 'rules' } }
  },
      {
        path: 'users',
        name: 'users',
        component: () => import('@/views/user/UserListView.vue'),
        meta: { title: '用户管理', requiresAdmin: true }
      },
      {
        path: 'telemetry/history',
        name: 'telemetry-history',
        component: () => import('@/views/telemetry/TelemetryHistoryView.vue'),
        meta: { title: '遥测历史' }
      },
      {
        path: 'music',
        name: 'music',
        component: () => import('@/views/music/MusicManagementView.vue'),
        meta: { title: '音乐管理', requiresAdmin: true }
      },
      {
        path: 'monitor',
        name: 'system-monitor',
        component: () => import('@/views/monitor/SystemMonitorView.vue'),
        meta: { title: '系统监控' }
      },
      {
        path: 'monitor/logs',
        name: 'system-logs',
        component: () => import('@/views/monitor/LogView.vue'),
        meta: { title: '事件日志' }
      },
      {
        path: 'settings',
        name: 'settings',
        component: () => import('@/views/settings/SystemSettingsView.vue'),
        meta: { title: '系统设置' }
      }
    ]
  }
];

const router = createRouter({
  history: createWebHistory(),
  routes
});

router.beforeEach((to, _from, next) => {
  const auth = useAuthStore();
  const isPublic = Boolean(to.meta.public);
  if (!isPublic && !auth.isAuthenticated) {
    next({ name: 'login', query: { redirect: to.fullPath } });
    return;
  }
  // Allow access for PRIMARY_ADMIN and SECONDARY_ADMIN (backward compatible with ROLE_ADMIN)
  if (to.meta.requiresAdmin) {
    const role = auth.user?.role;
    const isAdmin = role === 'ROLE_PRIMARY_ADMIN' || role === 'ROLE_SECONDARY_ADMIN' || role === 'ROLE_ADMIN';
    if (!isAdmin) {
      next({ name: 'dashboard' });
      return;
    }
  }
  if (to.meta.title) {
    document.title = `${to.meta.title} · 物联网云平台`;
  }
  next();
});

export default router;
