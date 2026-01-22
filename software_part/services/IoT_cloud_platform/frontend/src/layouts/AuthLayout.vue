<template>
  <div :class="['auth-shell', { compact, minimal: minimalEnv }]">
    <div class="panel">
      <div class="brand" v-if="!compact && !minimalEnv">
        <el-icon size="22"><Lock /></el-icon>
        <div>
          <p class="title">物联网云平台</p>
          <p class="subtitle">安全的设备运营中台</p>
        </div>
      </div>
      <RouterView />
    </div>

    <div v-if="!compact && !minimalEnv" class="hero-cta">
      <p class="hero-title">实时可见，可靠可控。</p>
      <p class="hero-desc">统一管理设备、监测指标并处理告警。</p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { useRoute } from 'vue-router';
import { Lock } from '@element-plus/icons-vue';

const route = useRoute();
const compact = computed(() => {
  if (route.meta?.compact) return true;
  if (route.query.compact === '1') return true;
  return import.meta.env.VITE_LOGIN_COMPACT === '1';
});
const minimalEnv = computed(() => import.meta.env.VITE_LOGIN_MINIMAL === '1');
</script>

<style scoped>

.auth-shell {
  position: relative;
  min-height: 100vh;
  display: flex;
  justify-content: center;
  align-items: center;
  padding: 32px 20px;
}

.auth-shell.compact {
  justify-content: center;
  align-items: center;
  background: radial-gradient(circle at 25% 20%, rgba(14, 165, 233, 0.08), transparent 40%),
    linear-gradient(135deg, var(--bg-primary), var(--bg-secondary));
  padding: 24px 16px;
}

.panel {
  background: var(--surface);
  padding: 48px 56px;
  display: flex;
  flex-direction: column;
  gap: 28px;
  box-shadow: var(--shadow-xl);
  transition: all var(--transition-base);
}

.auth-shell .panel {
  width: min(520px, 96vw);
  box-shadow: var(--shadow-lg);
  border: 1px solid var(--border-primary);
  position: relative;
  z-index: 2;
}

.auth-shell.minimal {
  padding: 0;
  background: transparent;
  min-height: 100vh;
  width: 100%;
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
}

.auth-shell.minimal .panel {
  width: 100%;
  max-width: none;
  min-height: 100vh;
  background: transparent;
  box-shadow: none;
  border: none;
  padding: 0;
  gap: 0;
  pointer-events: auto;
}

.hero-cta {
  position: absolute;
  right: max(5vw, 32px);
  bottom: max(10vh, 40px);
  max-width: 460px;
  color: var(--text-inverse);
  text-align: left;
  z-index: 1;
}

.hero-title {
  margin: 0;
  font-size: 32px;
  font-weight: 700;
}

.hero-desc {
  margin: 0;
  color: rgba(255, 255, 255, 0.85);
  max-width: 420px;
}

.brand {
  display: flex;
  align-items: center;
  gap: 12px;
  color: var(--text-primary);
}

.title {
  margin: 0;
  font-weight: 700;
  font-size: 18px;
  color: var(--text-primary);
}

.subtitle {
  margin: 4px 0 0;
  color: var(--text-secondary);
  font-size: 14px;
}

@media (max-width: 960px) {
  .hero-cta {
    display: none;
  }

  .auth-shell {
    padding: 18px 12px;
  }

  .auth-shell .panel {
    padding: 28px 24px;
  }
}
</style>
