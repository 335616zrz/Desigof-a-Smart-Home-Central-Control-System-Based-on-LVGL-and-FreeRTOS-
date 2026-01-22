<template>
  <div v-if="isInitialLoading" class="dashboard-shell page-padding loading-state">
    <el-skeleton :rows="8" animated />
  </div>
  <div v-else class="dashboard-shell page-padding" :key="renderKey">
    <el-alert v-if="!isOnline" type="warning" :closable="false" show-icon class="inline-alert">
      <template #title>网络连接已断开，部分功能可能无法正常使用</template>
    </el-alert>

    <section class="hero">
      <div class="hero__content">
        <p class="eyebrow">指挥中心</p>
        <h1>实时可见 · 可靠可控</h1>
        <p class="lede">统一管理设备、监测指标并处理告警。</p>

        <div class="hero__badges">
          <el-tag :type="telemetry.connected ? 'success' : 'warning'" effect="dark">
            MQTT {{ telemetry.connected ? '已连接' : '连接中…' }}
          </el-tag>
          <el-tag :type="isOnline ? 'success' : 'warning'" effect="plain">
            网络 {{ isOnline ? '在线' : '离线' }}
          </el-tag>
          <el-tag
            v-if="ruleStore.currentSeason"
            :type="seasonTagType(ruleStore.currentSeason.season)"
            effect="plain"
          >
            {{ ruleStore.currentSeason.seasonName }}告警模式
          </el-tag>
          <span class="last-update">
            最新数据：{{ lastTelemetryText }}
          </span>
        </div>

        <div class="hero__actions">
          <el-button type="primary" round @click="goDevices">添加设备</el-button>
          <el-button round plain @click="goAlerts">
            <el-icon style="margin-right: 4px;"><BellFilled /></el-icon>
            查看告警
          </el-button>
        </div>
      </div>

      <div class="hero__panel">
        <div class="online-block">
          <div class="label-row">
            <span>在线率</span>
            <el-tag size="small" effect="plain" type="success">{{ onlineRate }}%</el-tag>
          </div>
          <h2>{{ onlineRate }}%</h2>
          <el-progress :percentage="onlineRate" :stroke-width="10" striped striped-flow />
          <p class="hint">在线 {{ online }} / 总计 {{ totalDevices }}</p>
        </div>
        <div class="mini-grid">
          <div v-for="chip in heroChips" :key="chip.label" class="mini-card">
            <div class="chip-icon" :class="chip.tone">
              <el-icon :size="16"><component :is="chip.icon" /></el-icon>
            </div>
            <div class="mini-meta">
              <p>{{ chip.label }}</p>
              <strong>{{ chip.value }}</strong>
              <span>{{ chip.desc }}</span>
            </div>
          </div>
        </div>
      </div>
    </section>

    <section class="stat-grid">
      <div v-for="card in statCards" :key="card.label" class="stat-card">
        <div class="stat-icon" :class="card.tone">
          <el-icon><component :is="card.icon" /></el-icon>
        </div>
        <div class="stat-meta">
          <p class="muted">{{ card.label }}</p>
          <h3>{{ card.value }}</h3>
          <span>{{ card.sub }}</span>
        </div>
      </div>
    </section>

    <div class="chart-switch">
      <div class="chart-switch__left">
        <span class="label">数据视图</span>
        <el-radio-group v-model="chartMode" size="small">
          <el-radio-button label="live">实时（近10分钟）</el-radio-button>
          <el-radio-button label="history">历史（全量）</el-radio-button>
        </el-radio-group>
      </div>
      <div class="chart-switch__right">
        <el-tag size="small" effect="plain" type="info">温度 / 湿度</el-tag>
      </div>
    </div>

    <section class="panel-grid">
      <TelemetryChart
        class="grid-item"
        title="温度"
        :metric="'temperature'"
        :data-series="chartSeries"
        :show-thresholds="true"
        :x-axis-min="chartXAxisMin"
        :x-axis-max="chartXAxisMax"
      />
      <TelemetryChart
        class="grid-item"
        title="湿度"
        :metric="'humidity'"
        :data-series="chartSeries"
        :show-thresholds="true"
        :x-axis-min="chartXAxisMin"
        :x-axis-max="chartXAxisMax"
      />

      <el-card shadow="never" class="summary-card grid-item">
        <template #header>
          <div class="flex-between">
            <span>告警摘要</span>
            <el-tag size="small" type="warning" effect="plain">实时</el-tag>
          </div>
        </template>
        <el-timeline v-if="alertTimeline.length" class="alert-timeline">
          <el-timeline-item
            v-for="item in alertTimeline"
            :key="item.id"
            :timestamp="formatDateTime(item.createdAt as number)"
            :type="item.level === 'critical' ? 'danger' : item.level === 'warning' ? 'warning' : 'info'"
          >
            <div class="timeline-row">
              <span class="timeline-device">{{ item.deviceId }}</span>
              <span class="timeline-msg">{{ item.message }}</span>
            </div>
          </el-timeline-item>
        </el-timeline>
        <el-empty v-else description="当前没有告警" />
      </el-card>

      <el-card shadow="never" class="latest-card grid-item">
        <template #header>
          <div class="flex-between">
            <span>最新遥测</span>
            <el-link type="primary" @click="goHistory">查看历史</el-link>
          </div>
        </template>
        <el-table :data="latest" size="small" height="320">
          <el-table-column prop="deviceId" label="设备" width="120" />
          <el-table-column prop="temperature" label="温度(°C)" width="100" />
          <el-table-column prop="humidity" label="湿度(%)" width="100" />
          <el-table-column prop="pressure" label="气压" width="100" class-name="mobile-hide" />
          <el-table-column prop="timestamp" label="时间" :formatter="fmt" min-width="140" class-name="mobile-hide" />
        </el-table>
      </el-card>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed, nextTick, onMounted, onUnmounted, ref } from 'vue';
import { useRouter } from 'vue-router';
import { useDeviceStore } from '@/stores/device';
import { useTelemetryStream } from '@/stores/telemetryStream';
import { useTelemetryStore } from '@/stores/telemetry';
import { useAlertStore } from '@/stores/alert';
import { useAppStore } from '@/stores/app';
import TelemetryChart from '@/components/TelemetryChart.vue';
import { initMqtt } from '@/services/mqtt';
import { formatDateTime, toLocalIsoDateTime } from '@/utils/format';
import { useNetworkStatus } from '@/utils/useNetworkStatus';
import { useAlertRuleStore } from '@/stores/alertRule';
import dayjs from 'dayjs';
import {
  BellFilled,
  Connection,
  Cpu,
  Histogram,
  Link,
  SuccessFilled,
  WarningFilled
} from '@element-plus/icons-vue';

const router = useRouter();
const deviceStore = useDeviceStore();
const alertStore = useAlertStore();
const telemetry = useTelemetryStream();
const telemetryHistory = useTelemetryStore();
const appStore = useAppStore();
const ruleStore = useAlertRuleStore();
const { isOnline } = useNetworkStatus();
const chartMode = ref<'live' | 'history'>('live');

// 追踪初始数据加载状态
const isInitialLoading = ref(true);
// 渲染key，用于强制重新渲染以修复Grid布局
const renderKey = ref(0);
const nowTs = ref(Date.now());
let nowTimer: number | null = null;

const totalDevices = computed(() => deviceStore.items.length);
const online = computed(() => deviceStore.items.filter(d => d.status === 'ONLINE').length);
const offline = computed(() => deviceStore.items.filter(d => d.status === 'OFFLINE').length);
const openAlerts = computed(() => alertStore.items.length);
const criticalAlerts = computed(() => alertStore.items.filter(a => a.level === 'critical' && a.status === 'open').length);

const onlineRate = computed(() => {
  if (!totalDevices.value) return 0;
  return Math.round((online.value / totalDevices.value) * 100);
});

const lastTelemetryAt = computed(() => {
  const last = telemetry.points[telemetry.points.length - 1];
  return last ? last.timestamp : null;
});
const lastTelemetryText = computed(() =>
  lastTelemetryAt.value ? formatDateTime(lastTelemetryAt.value) : '等待设备上报'
);

const heroChips = computed(() => [
  { label: '告警(关键)', value: criticalAlerts.value, desc: '优先处理', icon: WarningFilled, tone: 'danger' },
  { label: '当前在线', value: online.value, desc: '实时在线设备', icon: Connection, tone: 'success' },
  { label: '采样缓存', value: telemetry.points.length, desc: '内存中数据点', icon: Histogram, tone: 'info' }
]);

const statCards = computed(() => [
  { label: '设备总数', value: totalDevices.value, sub: '已登记设备', icon: Cpu, tone: 'info' },
  { label: '在线设备', value: online.value, sub: '实时在线', icon: SuccessFilled, tone: 'success' },
  { label: '离线设备', value: offline.value, sub: '待排查', icon: Link, tone: 'warning' },
  { label: '开放告警', value: openAlerts.value, sub: '处理中/未关闭', icon: BellFilled, tone: 'danger' }
]);

const liveWindowMs = 10 * 60 * 1000; // 最近 10 分钟
const chartSeries = computed(() => (chartMode.value === 'live' ? telemetry.points : telemetryHistory.history));
const chartXAxisMin = computed(() => (chartMode.value === 'live' ? nowTs.value - liveWindowMs : 'dataMin'));
const chartXAxisMax = computed(() => nowTs.value);

const latest = computed(() => {
  const map: Record<string, typeof telemetry.points[number]> = {};
  telemetry.points.forEach(p => {
    map[p.deviceId] = p;
  });
  return Object.values(map).sort((a, b) => b.timestamp - a.timestamp);
});

const fmt = (_: unknown, __: unknown, cell: number) => formatDateTime(cell);

function goHistory() {
  router.push('/telemetry/history');
}

function goDevices() {
  router.push('/devices');
}

function goAlerts() {
  router.push('/alerts');
}

function seasonTagType(season: string): 'danger' | 'warning' | 'info' {
  const types: Record<string, 'danger' | 'warning' | 'info'> = {
    summer: 'danger',
    winter: 'info',
    transition: 'warning'
  };
  return types[season] || 'info';
}

onMounted(() => {
  nowTimer = window.setInterval(() => {
    nowTs.value = Date.now();
  }, 5000);

  // 并行加载所有数据，避免串行阻塞导致UI渲染延迟
  Promise.all([
    deviceStore.fetchList({ pageSize: 100 }),
    alertStore.fetchList({ pageSize: 50, status: 'open' }),
    ruleStore.fetchCurrentSeason()
  ])
    .then(() => {
      // 计算所有设备的最早注册时间
      const earliestCreatedAt = deviceStore.items.reduce((earliest, device) => {
        if (!device.createdAt) return earliest;
        const deviceTime = new Date(device.createdAt).getTime();
        return deviceTime < earliest ? deviceTime : earliest;
      }, Date.now());

      // 从最早设备注册日期到现在加载历史遥测数据
      const from = toLocalIsoDateTime(earliestCreatedAt);
      const to = toLocalIsoDateTime(Date.now());

      // 加载历史遥测（从设备注册开始到现在）
      // 优化：减少初始查询大小以避免与小程序轮询产生数据库锁竞争
      // 实时数据由 MQTT 提供，历史模式下用户可按需加载更多
      return telemetryHistory.loadHistory({ from, to, pageSize: 500 });
    })
    .catch(error => {
      console.error('Failed to load dashboard data:', error);
    })
    .finally(() => {
      // 使用 nextTick 确保 DOM 完全更新后再移除骨架屏
      // 这样可以确保 Grid 布局有足够时间正确计算
      nextTick(() => {
        // 添加短暂延迟，确保浏览器完成布局计算
        setTimeout(() => {
          isInitialLoading.value = false;
          // 强制重新渲染以确保Grid布局正确计算
          renderKey.value++;
        }, 100);
      });
    });

  // MQTT连接可以独立初始化，不需要等待其他数据
  initMqtt({ host: appStore.mqttWsUrl });
});

onUnmounted(() => {
  if (nowTimer) window.clearInterval(nowTimer);
  nowTimer = null;
});

// MQTT 连接由全局单例维护，避免路由切换导致实时数据中断/丢失。
// 如需断开，可在退出登录时统一处理。

const alertTimeline = computed(() =>
  [...alertStore.items]
    .filter(item => item.status === 'open')
    .sort((a, b) => dayjs(b.createdAt ?? b.timestamp ?? 0).valueOf() - dayjs(a.createdAt ?? a.timestamp ?? 0).valueOf())
    .slice(0, 5)
);
</script>

<style scoped>
.dashboard-shell {
  max-width: 1680px;
  margin: 0 auto;
  display: flex;
  flex-direction: column;
  gap: 18px;
}

.loading-state {
  min-height: 60vh;
  display: flex;
  align-items: center;
  justify-content: center;
}

.inline-alert {
  margin-bottom: 4px;
  border-radius: 16px;
}

.hero {
  position: relative;
  width: 100%;
  box-sizing: border-box;
  display: grid;
  /* 使用更明确的列定义，确保在>=768px时显示为两列 */
  grid-template-columns: 1fr 1fr;
  gap: 18px;
  padding: 26px 28px;
  border-radius: 28px;
  background: radial-gradient(circle at 20% 20%, rgba(10, 168, 255, 0.12), transparent 45%),
    radial-gradient(circle at 82% 0%, rgba(99, 102, 241, 0.12), transparent 40%),
    linear-gradient(135deg, rgba(255, 255, 255, 0.92), rgba(240, 247, 255, 0.96));
  border: 1px solid rgba(120, 146, 180, 0.18);
  box-shadow: 0 16px 42px rgba(40, 70, 130, 0.14);
  overflow: hidden;
}

body.dark .hero {
  background: radial-gradient(circle at 20% 20%, rgba(56, 189, 248, 0.14), transparent 45%),
    radial-gradient(circle at 82% 0%, rgba(79, 70, 229, 0.12), transparent 40%),
    linear-gradient(135deg, rgba(24, 32, 48, 0.92), rgba(16, 26, 42, 0.94));
  border-color: rgba(255, 255, 255, 0.08);
  box-shadow: 0 16px 42px rgba(0, 0, 0, 0.35);
}

.hero::after {
  content: '';
  position: absolute;
  inset: 0;
  background: linear-gradient(120deg, rgba(255, 255, 255, 0.35), rgba(255, 255, 255, 0.08));
  opacity: 0.65;
  pointer-events: none;
  mix-blend-mode: screen;
}

.hero__content {
  position: relative;
  z-index: 1;
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.eyebrow {
  margin: 0;
  letter-spacing: 0.08em;
  font-weight: 700;
  color: #0aa8ff;
  text-transform: uppercase;
}

.hero h1 {
  margin: 0;
  font-size: clamp(26px, 3vw, 34px);
  font-weight: 800;
  letter-spacing: -0.3px;
  color: var(--text-primary);
}

.lede {
  margin: 0;
  color: var(--text-secondary);
  max-width: 560px;
}

.hero__badges {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
}

.last-update {
  color: var(--text-secondary);
  font-size: 14px;
}

.hero__actions {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

.hero__panel {
  position: relative;
  z-index: 1;
  background: rgba(255, 255, 255, 0.72);
  border: 1px solid rgba(120, 146, 180, 0.18);
  border-radius: 22px;
  padding: 16px;
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.8), 0 10px 28px rgba(40, 70, 130, 0.16);
  display: flex;
  flex-direction: column;
  gap: 12px;
  backdrop-filter: blur(16px);
  -webkit-backdrop-filter: blur(16px);
}

body.dark .hero__panel {
  background: rgba(25, 35, 52, 0.78);
  border-color: rgba(255, 255, 255, 0.08);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.04), 0 10px 28px rgba(0, 0, 0, 0.35);
}

.online-block {
  background: linear-gradient(135deg, rgba(10, 168, 255, 0.12), rgba(59, 130, 246, 0.12));
  border-radius: 16px;
  padding: 12px 14px;
  border: 1px solid rgba(10, 168, 255, 0.18);
}

body.dark .online-block {
  background: linear-gradient(135deg, rgba(56, 189, 248, 0.16), rgba(14, 165, 233, 0.12));
  border-color: rgba(255, 255, 255, 0.08);
}

.label-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  color: var(--text-secondary);
}

.online-block h2 {
  margin: 6px 0 6px;
  font-size: 30px;
  color: var(--text-primary);
  letter-spacing: -0.2px;
}

.hint {
  margin: 4px 0 0;
  color: var(--text-secondary);
  font-size: 13px;
}

.mini-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));
  gap: 10px;
}

.mini-card {
  display: flex;
  gap: 10px;
  align-items: center;
  padding: 10px 12px;
  border: 1px solid rgba(120, 146, 180, 0.16);
  border-radius: 14px;
  background: var(--surface);
  box-shadow: 0 8px 18px rgba(40, 70, 130, 0.08);
}

body.dark .mini-card {
  border-color: rgba(255, 255, 255, 0.08);
}

.chip-icon {
  width: 32px;
  height: 32px;
  border-radius: 10px;
  display: grid;
  place-items: center;
  color: #fff;
}

.chip-icon.success { background: linear-gradient(135deg, #10b981, #16a34a); }
.chip-icon.danger { background: linear-gradient(135deg, #ef4444, #b91c1c); }
.chip-icon.info { background: linear-gradient(135deg, #0aa8ff, #0369a1); }
.chip-icon.warning { background: linear-gradient(135deg, #f59e0b, #d97706); }

.mini-meta p {
  margin: 0;
  color: var(--text-secondary);
  font-size: 13px;
}

.mini-meta strong {
  display: block;
  font-size: 18px;
  color: var(--text-primary);
  margin: 2px 0;
}

.mini-meta span {
  color: var(--text-tertiary);
  font-size: 12px;
}

.stat-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
  gap: 14px;
}

.stat-card {
  position: relative;
  padding: 18px;
  border-radius: 18px;
  border: 1px solid rgba(120, 146, 180, 0.18);
  background: rgba(255, 255, 255, 0.9);
  box-shadow: 0 12px 28px rgba(40, 70, 130, 0.12);
  display: grid;
  grid-template-columns: auto 1fr;
  gap: 12px;
  align-items: center;
  overflow: hidden;
  transition: transform 0.18s ease, box-shadow 0.18s ease, border-color 0.18s ease;
}

.stat-card::after {
  content: '';
  position: absolute;
  inset: 0;
  background: linear-gradient(120deg, rgba(10, 168, 255, 0.08), transparent 60%);
  opacity: 0;
  transition: opacity 0.18s ease;
}

.stat-card:hover {
  transform: translateY(-3px);
  box-shadow: 0 16px 36px rgba(40, 70, 130, 0.16);
  border-color: rgba(10, 168, 255, 0.36);
}

.stat-card:hover::after {
  opacity: 1;
}

body.dark .stat-card {
  background: rgba(28, 39, 58, 0.9);
  border-color: rgba(255, 255, 255, 0.08);
  box-shadow: 0 12px 28px rgba(0, 0, 0, 0.35);
}

.stat-icon {
  width: 46px;
  height: 46px;
  border-radius: 14px;
  display: grid;
  place-items: center;
  color: #fff;
  z-index: 1;
}

.stat-icon.info { background: linear-gradient(135deg, #0aa8ff, #0369a1); }
.stat-icon.success { background: linear-gradient(135deg, #10b981, #16a34a); }
.stat-icon.warning { background: linear-gradient(135deg, #f59e0b, #d97706); }
.stat-icon.danger { background: linear-gradient(135deg, #ef4444, #b91c1c); }

.stat-meta {
  z-index: 1;
}

.stat-meta h3 {
  margin: 6px 0;
  font-size: 26px;
  color: var(--text-primary);
}

.stat-meta span {
  color: var(--text-secondary);
  font-size: 13px;
}

.panel-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
  gap: 14px;
}

.chart-switch {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 12px;
  padding: 4px 4px 0;
}

.chart-switch__left {
  display: flex;
  align-items: center;
  gap: 10px;
  flex-wrap: wrap;
}

.chart-switch .label {
  font-weight: 600;
  color: var(--text-secondary);
}

/* 暗夜模式下修正「实时/历史」切换按钮的对比度 */
body.dark .chart-switch :deep(.el-radio-button__inner) {
  background-color: rgba(30, 41, 59, 0.9);
  color: var(--text-secondary);
  border-color: rgba(255, 255, 255, 0.14);
}

body.dark .chart-switch :deep(.el-radio-button__inner:hover) {
  background-color: rgba(51, 65, 85, 0.9);
  color: var(--text-primary);
}

body.dark .chart-switch :deep(.el-radio-button__original-radio:checked + .el-radio-button__inner) {
  background: linear-gradient(135deg, #0aa8ff, #0b77d6);
  border-color: transparent;
  color: #fff;
  box-shadow: 0 4px 10px rgba(10, 168, 255, 0.35);
}

.grid-item {
  width: 100%;
}

.summary-card,
.latest-card :deep(.el-card__header) {
  border-radius: 18px;
}

.alert-timeline {
  max-height: 320px;
  overflow: auto;
}

.timeline-row {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.timeline-device {
  font-weight: 700;
  color: var(--text-primary);
}

.timeline-msg {
  color: var(--text-secondary);
}

.flex-between {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

@media (max-width: 768px) {
  .hero {
    /* 小屏幕时改为单列布局 */
    grid-template-columns: 1fr;
    padding: 18px;
    border-radius: 20px;
  }

  .stat-card {
    padding: 16px;
  }

  .hero__actions {
    flex-direction: column;
    width: 100%;
  }

  .hero__actions .el-button {
    width: 100%;
  }

  .chart-switch {
    flex-direction: column;
    align-items: stretch;
  }

  .chart-switch__left {
    flex-direction: column;
    gap: 8px;
  }

  .chart-switch .el-radio-group {
    width: 100%;
  }

  .panel-grid {
    grid-template-columns: 1fr;
  }

  .latest-card .el-table {
    font-size: 13px;
  }

  .latest-card .el-table th,
  .latest-card .el-table td {
    padding: 8px 4px;
  }
}
</style>
