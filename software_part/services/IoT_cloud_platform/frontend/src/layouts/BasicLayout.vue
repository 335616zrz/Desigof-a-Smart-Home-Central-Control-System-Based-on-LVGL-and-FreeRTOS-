<template>
  <el-container class="layout">
    <!-- 移动端遮罩层 -->
    <div v-if="sidebarVisible" class="mobile-overlay" @click="toggleMobileSidebar"></div>

    <el-aside :width="collapsed ? '72px' : '220px'" class="aside" :class="{ 'mobile-hidden': !sidebarVisible }">
      <div class="brand" @click="goHome">
        <el-icon size="20"><Monitor /></el-icon>
        <span v-if="!collapsed">物联网云平台</span>
      </div>
      <el-menu :default-active="active" class="menu" router :collapse="collapsed" background-color="transparent" @select="handleMenuSelect">
        <el-menu-item index="/">
          <el-icon><DataAnalysis /></el-icon>
          <span>仪表盘</span>
        </el-menu-item>
        <el-menu-item index="/devices">
          <el-icon><Cpu /></el-icon>
          <span>设备</span>
        </el-menu-item>
        <el-menu-item index="/alerts">
          <el-icon><Warning /></el-icon>
          <span>告警</span>
        </el-menu-item>
        <el-menu-item v-if="isAdmin" index="/music">
          <el-icon><Headset /></el-icon>
          <span>音乐管理</span>
        </el-menu-item>
        <el-menu-item index="/telemetry/history">
          <el-icon><TrendCharts /></el-icon>
          <span>遥测历史</span>
        </el-menu-item>
        <el-menu-item index="/monitor">
          <el-icon><Monitor /></el-icon>
          <span>系统监控</span>
        </el-menu-item>
        <el-menu-item index="/monitor/logs">
          <el-icon><TrendCharts /></el-icon>
          <span>事件日志</span>
        </el-menu-item>
        <el-menu-item v-if="isAdmin" index="/users">
          <el-icon><UserFilled /></el-icon>
          <span>用户管理</span>
        </el-menu-item>
        <el-menu-item index="/settings">
          <el-icon><Cpu /></el-icon>
          <span>系统设置</span>
        </el-menu-item>
      </el-menu>
      <div class="collapse" @click="toggle">
        <el-icon size="18"><Fold v-if="!collapsed" /><Expand v-else /></el-icon>
      </div>
    </el-aside>
    <el-container>
      <el-header class="header">
        <div class="left">
          <el-button class="mobile-menu-btn" circle @click="toggleMobileSidebar">
            <el-icon><Menu /></el-icon>
          </el-button>
          <el-breadcrumb separator="/">
            <el-breadcrumb-item v-for="item in crumbs" :key="item.path">{{ item.label }}</el-breadcrumb-item>
          </el-breadcrumb>
        </div>
        <div class="right">
          <div class="theme-toggle" aria-label="主题切换">
            <button
              v-for="mode in themeModes"
              :key="mode"
              :data-theme="mode"
              :class="{ active: themeMode === mode }"
              @click="setTheme(mode)"
            >
              <span v-if="mode === 'light'">☀️</span>
              <span v-else-if="mode === 'auto'">⏱️</span>
              <span v-else>🌙</span>
            </button>
          </div>

          <el-dropdown>
            <span class="user">
              <el-avatar size="small">{{ initials }}</el-avatar>
              <span class="username">{{ auth.user?.username || '用户' }}</span>
              <el-icon><ArrowDown /></el-icon>
            </span>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item @click="logout">退出登录</el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
        </div>
      </el-header>
      <el-main class="main">
        <RouterView v-slot="{ Component, route }">
          <transition name="page-fade" mode="out-in">
            <component :is="Component" :key="route.path" />
          </transition>
        </RouterView>
      </el-main>
    </el-container>
  </el-container>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { useAppStore } from '@/stores/app';
import { useAuthStore } from '@/stores/auth';
import { ArrowDown, Cpu, DataAnalysis, Expand, Fold, Headset, Monitor, TrendCharts, Warning, UserFilled, Menu } from '@element-plus/icons-vue';

const app = useAppStore();
const auth = useAuthStore();
const route = useRoute();
const router = useRouter();

const sidebarVisible = ref(false);

const collapsed = computed(() => app.collapsed);
const active = computed(() => {
  if (route.path.startsWith('/devices/')) return '/devices';
  if (route.path.startsWith('/alerts/')) return '/alerts';
  if (route.path.startsWith('/monitor/logs')) return '/monitor/logs';
  if (route.path.startsWith('/music')) return '/music';
  return route.path;
});
const themeModes = ['light', 'auto', 'dark'] as const;
const themeMode = computed(() => app.theme);
const setTheme = (mode: 'light' | 'auto' | 'dark') => app.setTheme(mode);

const initials = computed(() => (auth.user?.username || 'U').slice(0, 2).toUpperCase());
// Support new hierarchical admin system: PRIMARY_ADMIN, SECONDARY_ADMIN, and legacy ROLE_ADMIN
const isAdmin = computed(() => {
  const role = auth.user?.role;
  return role === 'ROLE_PRIMARY_ADMIN' || role === 'ROLE_SECONDARY_ADMIN' || role === 'ROLE_ADMIN';
});

const crumbs = computed(() => {
  const map: Record<string, string> = {
    '/': '仪表盘',
    '/devices': '设备',
    '/alerts': '告警',
    '/music': '音乐管理',
    '/telemetry/history': '遥测历史',
    '/monitor': '系统监控',
    '/monitor/logs': '事件日志',
    '/settings': '系统设置',
    '/users': '用户管理'
  };
  const segments = route.path.split('/').filter(Boolean);
  const list: { path: string; label: string }[] = [];
  let acc = '';
  segments.forEach(seg => {
    acc += `/${seg}`;
    list.push({ path: acc, label: map[acc] || seg });
  });
  if (!segments.length) list.push({ path: '/', label: '仪表盘' });
  return list;
});

function toggle() {
  app.toggleSidebar();
}

function toggleMobileSidebar() {
  sidebarVisible.value = !sidebarVisible.value;
}

// 点击菜单项后在移动端自动关闭侧边栏
function handleMenuSelect() {
  if (window.innerWidth <= 768) {
    sidebarVisible.value = false;
  }
}

function goHome() {
  router.push('/');
}

function logout() {
  auth.logout();
  router.push('/login');
}
</script>

<style>
/* 全局样式 - 不使用scoped以确保背景渐变生效 */
.el-container.layout {
  min-height: 100vh;
  background: radial-gradient(circle at 20% 20%, rgba(255,255,255,0.6), transparent 40%),
              radial-gradient(circle at 80% 0%, rgba(200,220,255,0.4), transparent 35%),
              linear-gradient(135deg, #f7f9fc, #eef2ff 50%, #f4f7ff) !important;
  transition: background var(--transition-base);
}

body.dark .el-container.layout {
  background: radial-gradient(circle at 20% 20%, rgba(255,255,255,0.03), transparent 40%),
              radial-gradient(circle at 80% 0%, rgba(100,140,200,0.06), transparent 35%),
              linear-gradient(135deg, #0f1626, #111c2f 50%, #0b1322) !important;
}
</style>

<style scoped>

.aside {
  background: rgba(255, 255, 255, 0.65);
  backdrop-filter: blur(24px) saturate(180%);
  -webkit-backdrop-filter: blur(24px) saturate(180%);
  color: var(--text-primary);
  display: flex;
  flex-direction: column;
  border-right: 1px solid rgba(255, 255, 255, 0.3);
  box-shadow: 4px 0 30px rgba(0, 0, 0, 0.03);
  transition: all var(--transition-base);
}

body.dark .aside {
  background: rgba(15, 23, 42, 0.65);
  border-right-color: rgba(255, 255, 255, 0.08);
  box-shadow: 4px 0 30px rgba(0, 0, 0, 0.3);
}

.brand {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 18px 16px;
  font-weight: 700;
  letter-spacing: 0.3px;
  cursor: pointer;
  color: var(--text-primary);
  transition: color var(--transition-fast);
}

.brand:hover {
  color: #0aa8ff;
}

.menu {
  flex: 1;
  border-right: none;
}

.menu :deep(.el-menu-item) {
  height: 42px;
  margin: 6px 10px;
  border-radius: 12px;
  padding-left: 12px !important;
  color: var(--text-primary);
}

body.dark .menu :deep(.el-menu-item) {
  color: var(--text-primary) !important;
}

.menu :deep(.el-menu-item.is-active) {
  background: linear-gradient(90deg, rgba(56, 189, 248, 0.15), rgba(56, 189, 248, 0.02)) !important;
  border-right: 3px solid var(--brand-500);
  color: var(--brand-700) !important;
  font-weight: 700;
  box-shadow: 0 6px 14px rgba(10, 168, 255, 0.12);
}

body.dark .menu :deep(.el-menu-item.is-active) {
  color: #7bc3ff !important;
  background: linear-gradient(90deg, rgba(56, 189, 248, 0.22), rgba(56, 189, 248, 0.08)) !important;
  border-right: 3px solid var(--brand-500);
  box-shadow: 0 6px 14px rgba(0, 0, 0, 0.28);
}

.collapse {
  padding: 12px 0;
  text-align: center;
  cursor: pointer;
  color: var(--text-tertiary);
  transition: color var(--transition-fast);
}

.collapse:hover {
  color: #0aa8ff;
}

.header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  background: rgba(255, 255, 255, 0.65);
  backdrop-filter: blur(24px) saturate(180%);
  -webkit-backdrop-filter: blur(24px) saturate(180%);
  border-bottom: 1px solid rgba(255, 255, 255, 0.3);
  padding: 10px 28px;
  box-shadow: 0 10px 30px rgba(0, 0, 0, 0.04);
  transition: all var(--transition-base);
}

body.dark .header {
  background: rgba(15, 23, 42, 0.65);
  border-bottom-color: rgba(255, 255, 255, 0.08);
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.28);
}

.right {
  display: flex;
  align-items: center;
  gap: 18px;
}

.user {
  display: flex;
  align-items: center;
  gap: 8px;
  cursor: pointer;
  color: var(--text-primary);
  transition: color var(--transition-fast);
}

.user:hover {
  color: #0aa8ff;
}

.theme-toggle {
  display: inline-flex;
  background: rgba(255, 255, 255, 0.9);
  border: 1px solid rgba(120, 146, 180, 0.18);
  border-radius: 999px;
  padding: 4px;
  gap: 4px;
  box-shadow: 0 8px 18px rgba(40, 70, 130, 0.12);
}

body.dark .theme-toggle {
  background: rgba(30, 41, 59, 0.9);
  border-color: rgba(255, 255, 255, 0.1);
  box-shadow: 0 8px 18px rgba(0, 0, 0, 0.25);
}

.theme-toggle button {
  border: none;
  background: transparent;
  width: 34px;
  height: 30px;
  border-radius: 12px;
  display: grid;
  place-items: center;
  cursor: pointer;
  color: var(--text-secondary);
  transition: all var(--transition-fast);
}

.theme-toggle button.active {
  background: linear-gradient(135deg, #0aa8ff, #0b77d6);
  color: #fff;
  box-shadow: 0 6px 12px rgba(10, 168, 255, 0.25);
}

.username {
  font-weight: 600;
  color: var(--text-primary);
}

.main {
  background: transparent;
  min-height: calc(100vh - 60px);
  transition: background-color var(--transition-base);
}

/* 移动端遮罩层 */
.mobile-overlay {
  display: none;
}

/* 移动端适配 */
.mobile-menu-btn {
  display: none;
  margin-right: 12px;
}

@media (max-width: 768px) {
  .mobile-menu-btn {
    display: inline-flex;
  }

  .mobile-overlay {
    display: block;
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background: rgba(0, 0, 0, 0.5);
    z-index: 999;
    animation: fadeIn var(--transition-base);
  }

  @keyframes fadeIn {
    from {
      opacity: 0;
    }
    to {
      opacity: 1;
    }
  }

  .aside {
    position: fixed;
    left: 0;
    top: 0;
    bottom: 0;
    z-index: 1000;
    width: 280px !important;
    transition: transform var(--transition-base);
    box-shadow: 4px 0 24px rgba(0, 0, 0, 0.15);
  }

  body.dark .aside {
    box-shadow: 4px 0 24px rgba(0, 0, 0, 0.5);
  }

  .aside.mobile-hidden {
    transform: translateX(-100%);
  }

  .el-container {
    width: 100%;
  }

  .left {
    display: flex;
    align-items: center;
  }

  .el-breadcrumb {
    display: none;
  }

  .header {
    padding: 10px 16px;
  }

  .right {
    gap: 12px;
  }
}

@media (min-width: 769px) {
  .aside {
    position: relative;
  }

  .aside.mobile-hidden {
    transform: none;
  }
}

</style>
