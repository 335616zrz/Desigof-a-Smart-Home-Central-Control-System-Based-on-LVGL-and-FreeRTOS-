<template>
  <div class="page-shell page-padding">
    <section class="app-hero">
      <div class="app-hero__content">
        <p class="eyebrow">系统监控</p>
        <h1>后端健康 · 资源概览</h1>
        <p class="lede">快速了解健康状态、资源占用与外部监控入口。</p>

        <div class="app-hero__badges">
          <el-tag :type="healthStatusTag" effect="dark">{{ healthStatusText }}</el-tag>
          <el-tag effect="plain" type="success">在线设备 {{ deviceOnline }}/{{ deviceTotal }}</el-tag>
          <el-tag effect="plain" type="warning">开放告警 {{ openAlerts }}</el-tag>
        </div>

        <div class="app-hero__actions">
          <el-button type="primary" @click="load" :loading="loading">刷新</el-button>
          <el-button plain @click="openLink(grafanaUrl)">打开 Grafana</el-button>
          <el-button plain @click="openLink(prometheusUrl)">打开 Prometheus</el-button>
        </div>
      </div>

      <div class="app-hero__panel">
        <div class="app-mini-grid">
          <div class="app-mini-card">
            <div class="app-chip-icon success"><el-icon><Cpu /></el-icon></div>
            <div class="mini-meta">
              <p>CPU 使用率</p>
              <strong>{{ (cpuUsage * 100).toFixed(1) }}%</strong>
              <span>动态刷新</span>
            </div>
          </div>
          <div class="app-mini-card">
            <div class="app-chip-icon info"><el-icon><Monitor /></el-icon></div>
            <div class="mini-meta">
              <p>内存占用</p>
              <strong>{{ memoryPercent }}%</strong>
              <span>{{ (memoryUsed / 1024 / 1024).toFixed(0) }} / {{ (memoryMax / 1024 / 1024).toFixed(0) }} MB</span>
            </div>
          </div>
          <div class="app-mini-card">
            <div class="app-chip-icon warning"><el-icon><Warning /></el-icon></div>
            <div class="mini-meta">
              <p>组件数</p>
              <strong>{{ serviceRows.length }}</strong>
              <span>/actuator/health</span>
            </div>
          </div>
        </div>
      </div>
    </section>

    <el-row :gutter="14">
      <el-col :xs="24" :md="12">
        <el-card shadow="never">
          <template #header><span>后端健康</span></template>
          <div class="status-row">
            <el-tag :type="healthStatusTag" effect="dark" size="large">
              {{ healthStatusText }}
            </el-tag>
            <span class="muted">来源：/actuator/health</span>
          </div>
          <el-table :data="serviceRows" size="small" style="margin-top: 8px;">
            <el-table-column prop="name" label="组件" width="140" />
            <el-table-column label="状态" width="120">
              <template #default="{ row }">
                <el-tag :type="row.status === 'UP' ? 'success' : 'danger'">{{ row.status }}</el-tag>
              </template>
            </el-table-column>
            <el-table-column prop="detail" label="详情" />
          </el-table>
        </el-card>
      </el-col>

      <el-col :xs="24" :md="12">
        <el-card shadow="never">
          <template #header><span>系统资源</span></template>
          <div class="metric">
            <div class="metric-row">
              <span>CPU 使用率</span>
              <strong>{{ (cpuUsage * 100).toFixed(1) }}%</strong>
            </div>
            <el-progress :percentage="Math.min(cpuUsage * 100, 100)" :status="cpuUsage > 0.8 ? 'exception' : 'success'" />
          </div>
          <div class="metric">
            <div class="metric-row">
              <span>JVM 内存</span>
              <strong>{{ (memoryUsed / 1024 / 1024).toFixed(0) }} / {{ (memoryMax / 1024 / 1024).toFixed(0) }} MB</strong>
            </div>
            <el-progress
              :percentage="memoryPercent"
              :status="memoryPercent > 80 ? 'exception' : 'success'"
              :color="memoryPercent > 80 ? '#f56c6c' : '#67c23a'"
            />
          </div>
        </el-card>
      </el-col>
    </el-row>

    <el-row :gutter="14" style="margin-top: 12px;">
      <el-col :xs="24" :md="12">
        <el-card shadow="never">
          <template #header><span>监控与可观测性</span></template>
          <div class="links">
            <el-button type="primary" plain @click="openLink(prometheusUrl)">Prometheus</el-button>
            <el-button type="primary" plain @click="openLink(grafanaUrl)">Grafana</el-button>
            <el-button type="primary" plain @click="openLink(emqxUrl)">EMQX 控制台</el-button>
          </div>
          <p class="muted">链接基于 .env 配置，可在系统设置中修改。</p>
        </el-card>
      </el-col>
      <el-col :xs="24" :md="12">
        <el-card shadow="never">
          <template #header><span>快速概览</span></template>
          <div class="summary">
            <div>
              <p class="muted">设备总数</p>
              <h3>{{ deviceTotal }}</h3>
            </div>
            <div>
              <p class="muted">在线设备</p>
              <h3>{{ deviceOnline }}</h3>
            </div>
            <div>
              <p class="muted">未处理告警</p>
              <h3>{{ openAlerts }}</h3>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <el-row :gutter="14" style="margin-top: 12px;">
      <el-col :xs="24" :md="12">
        <el-card shadow="never">
          <template #header><span>Prometheus 图表</span></template>
          <iframe v-if="embedAllowed" :src="prometheusUrl" class="embed" title="Prometheus" />
          <img v-else-if="prometheusSnapshot" :src="prometheusSnapshot" class="embed" alt="Prometheus snapshot" />
          <p v-else class="muted">跨域限制无法内嵌，点击上方按钮或配置 VITE_PROM_SNAPSHOT_URL。</p>
        </el-card>
      </el-col>
      <el-col :xs="24" :md="12">
        <el-card shadow="never">
          <template #header><span>Grafana 仪表盘预览</span></template>
          <iframe v-if="embedAllowed" :src="grafanaUrl" class="embed" title="Grafana" />
          <img v-else-if="grafanaSnapshot" :src="grafanaSnapshot" class="embed" alt="Grafana snapshot" />
          <p v-else class="muted">跨域限制无法内嵌，点击上方按钮或配置 VITE_GRAFANA_SNAPSHOT_URL。</p>
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue';
import { Cpu, Monitor, Warning } from '@element-plus/icons-vue';
import { useDeviceStore } from '@/stores/device';
import { useAlertStore } from '@/stores/alert';
import { fetchHealth, fetchMetric } from '@/api/monitor';

const deviceStore = useDeviceStore();
const alertStore = useAlertStore();

const health = ref<any>(null);
const cpuUsage = ref(0);
const memoryUsed = ref(0);
const memoryMax = ref(1);
const loading = ref(false);
let timer: number | undefined;

const serviceRows = computed(() => {
  const comps = health.value?.components || {};
  return Object.entries(comps).map(([name, value]: [string, any]) => ({
    name,
    status: value.status || value?.details?.status || 'UNKNOWN',
    detail: value.details?.error || value.details?.message || ''
  }));
});

const memoryPercent = computed(() => Math.min(100, Number(((memoryUsed.value / memoryMax.value) * 100).toFixed(1))));
const deviceTotal = computed(() => deviceStore.items.length);
const deviceOnline = computed(() => deviceStore.items.filter(d => d.status === 'ONLINE').length);
const openAlerts = computed(() => alertStore.items.filter(a => a.status === 'open').length);
const healthStatusText = computed(() => health.value?.status || 'UNKNOWN');
const healthStatusTag = computed(() => (health.value?.status === 'UP' ? 'success' : 'danger'));

const prometheusUrl = import.meta.env.VITE_PROMETHEUS_URL || 'http://localhost:9091';
const grafanaUrl = import.meta.env.VITE_GRAFANA_URL || 'http://localhost:3000';
const emqxUrl = import.meta.env.VITE_EMQX_URL || 'http://localhost:18083';
const prometheusSnapshot = import.meta.env.VITE_PROM_SNAPSHOT_URL || '';
const grafanaSnapshot = import.meta.env.VITE_GRAFANA_SNAPSHOT_URL || '';
const embedAllowed = import.meta.env.VITE_EMBED_MONITOR === 'true';

async function load() {
  loading.value = true;
  try {
    [health.value] = await Promise.all([
      fetchHealth(),
      deviceStore.fetchList({ pageSize: 50 }),
      alertStore.fetchList({ pageSize: 50 })
    ]);
    const cpu = await fetchMetric('system.cpu.usage');
    const memUsed = await fetchMetric('jvm.memory.used');
    const memMaxRes = await fetchMetric('jvm.memory.max');
    cpuUsage.value = cpu.measurements?.[0]?.value ?? 0;
    memoryUsed.value = memUsed.measurements?.find((m: any) => m.statistic === 'VALUE')?.value ?? 0;
    memoryMax.value = memMaxRes.measurements?.find((m: any) => m.statistic === 'VALUE')?.value ?? 1;
  } finally {
    loading.value = false;
  }
}

function openLink(url: string) {
  window.open(url, '_blank');
}

onMounted(() => {
  load();
  timer = window.setInterval(load, 30000);
});

onBeforeUnmount(() => {
  if (timer) window.clearInterval(timer);
});
</script>

<style scoped>
.status-row {
  display: flex;
  align-items: center;
  gap: 10px;
}
.metric {
  margin-bottom: 12px;
}
.metric-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
.links {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}
.summary {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 12px;
}
.summary h3 {
  margin: 4px 0 0;
}
.embed {
  width: 100%;
  height: 260px;
  border: 1px solid var(--border, #e5e7eb);
  border-radius: 10px;
}
</style>
