<template>
  <div class="page-shell page-padding">
    <section class="app-hero">
      <div class="app-hero__content">
        <p class="eyebrow">事件日志</p>
        <h1>实时流 · 一键回放</h1>
        <p class="lede">支持 SSE + MQTT 实时、关键字过滤与后端日志拉取。</p>

        <div class="app-hero__badges">
          <el-tag effect="dark" type="info">总计 {{ eventStore.items.length }}</el-tag>
          <el-tag effect="plain" type="warning">自动滚动 {{ autoScroll ? '开' : '关' }}</el-tag>
        </div>

        <div class="app-hero__actions">
          <el-switch v-model="autoScroll" active-text="自动滚动" />
          <el-button @click="pullLog" :loading="pulling">拉取后端日志</el-button>
          <el-button type="primary" @click="clear">清空</el-button>
        </div>
      </div>

      <div class="app-hero__panel">
        <div class="app-mini-grid">
          <div class="app-mini-card">
            <div class="app-chip-icon info"><el-icon><ChatLineRound /></el-icon></div>
            <div class="mini-meta">
              <p>遥测</p>
              <strong>{{ typeCount('telemetry') }}</strong>
              <span>数据点事件</span>
            </div>
          </div>
          <div class="app-mini-card">
            <div class="app-chip-icon warning"><el-icon><Bell /></el-icon></div>
            <div class="mini-meta">
              <p>告警</p>
              <strong>{{ typeCount('alert') }}</strong>
              <span>触发/关闭</span>
            </div>
          </div>
          <div class="app-mini-card">
            <div class="app-chip-icon success"><el-icon><Promotion /></el-icon></div>
            <div class="mini-meta">
              <p>系统</p>
              <strong>{{ typeCount('system') }}</strong>
              <span>日志/心跳</span>
            </div>
          </div>
        </div>
      </div>
    </section>

    <el-card shadow="never">
      <div class="app-section-card" style="margin-bottom: 10px; display:flex; gap:10px; flex-wrap:wrap; align-items:center;">
        <el-select v-model="filter.type" placeholder="全部类型" clearable style="width: 150px">
          <el-option label="遥测" value="telemetry" />
          <el-option label="告警" value="alert" />
          <el-option label="系统" value="system" />
        </el-select>
        <el-input v-model="filter.keyword" placeholder="设备ID/消息关键词" clearable style="width: 240px" />
        <el-button type="primary" @click="pullLog" :loading="pulling">拉取后端日志</el-button>
      </div>

      <el-table
        ref="tableRef"
        :data="filtered"
        height="560"
        size="small"
        border
        :row-class-name="rowClass"
      >
        <el-table-column prop="timestamp" label="时间" width="180" :formatter="fmt" />
        <el-table-column prop="type" label="类型" width="110">
          <template #default="{ row }">
            <el-tag :type="typeTag(row.type)">{{ typeLabel(row.type) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="deviceId" label="设备" width="140" />
        <el-table-column prop="message" label="消息" />
        <el-table-column prop="payload" label="详情">
          <template #default="{ row }">
            <el-popover placement="top-start" :width="360" trigger="click">
              <pre class="payload">{{ pretty(row.payload) }}</pre>
              <template #reference>
                <el-link type="primary" :underline="false">查看</el-link>
              </template>
            </el-popover>
          </template>
        </el-table-column>
      </el-table>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { computed, nextTick, onMounted, reactive, ref, watch } from 'vue';
import { onBeforeUnmount } from 'vue';
import { Bell, ChatLineRound, Promotion } from '@element-plus/icons-vue';
import { useEventStore } from '@/stores/event';
import { useAppStore } from '@/stores/app';
import { initMqtt } from '@/services/mqtt';
import { formatDateTime } from '@/utils/format';
import { fetchActuatorLogfile } from '@/api/logs';
import { openEventStream } from '@/api/events';

const eventStore = useEventStore();
const appStore = useAppStore();
const tableRef = ref();
const autoScroll = ref(true);
const filter = reactive<{ type?: string; keyword: string }>({ type: undefined, keyword: '' });
const pulling = ref(false);
let es: EventSource | null = null;

const filtered = computed(() =>
  eventStore.items.filter(ev => {
    const matchType = filter.type ? ev.type === filter.type : true;
    const kw = filter.keyword?.trim().toLowerCase();
    const matchKw = !kw || ev.message.toLowerCase().includes(kw) || ev.deviceId?.toLowerCase().includes(kw);
    return matchType && matchKw;
  })
);

watch(
  () => eventStore.items.length,
  async () => {
    if (!autoScroll.value) return;
    await nextTick();
    const body = tableRef.value?.$refs?.bodyWrapper as HTMLElement | undefined;
    if (body) body.scrollTop = 0;
  }
);

function clear() {
  eventStore.clear();
}

async function pullLog() {
  pulling.value = true;
  try {
    const text = await fetchActuatorLogfile();
    const lines = text.split('\n').slice(-200);
    lines
      .filter(Boolean)
      .reverse()
      .forEach(line =>
        eventStore.push({
          id: `log-${Math.random().toString(16).slice(2)}`,
          type: 'system',
          message: line,
          timestamp: Date.now()
        })
      );
  } finally {
    pulling.value = false;
  }
}

const fmt = (_: unknown, __: unknown, value: number) => formatDateTime(value);
const typeLabel = (type: string) => (type === 'telemetry' ? '遥测' : type === 'alert' ? '告警' : '系统');
const typeTag = (type: string) => (type === 'telemetry' ? 'info' : type === 'alert' ? 'warning' : 'success');
const pretty = (payload: any) => (payload ? JSON.stringify(payload, null, 2) : '无');
const rowClass = ({ row }: any) => (row.type === 'alert' ? 'row-alert' : row.type === 'system' ? 'row-system' : '');
const typeCount = (t: string) => eventStore.items.filter(ev => ev.type === t).length;

onMounted(() => {
  initMqtt({ host: appStore.mqttWsUrl });
  startSse();
});

onBeforeUnmount(() => {
  es?.close();
});

function startSse() {
  es?.close();
  es = openEventStream(appStore.apiBase);
  es.onmessage = ev => {
    eventStore.push({
      id: `sse-${Date.now()}`,
      type: 'system',
      message: ev.data,
      timestamp: Date.now()
    });
  };
  es.onerror = () => {
    es?.close();
  };
}
</script>

<style scoped>
.payload {
  margin: 0;
  white-space: pre-wrap;
  font-family: Menlo, Monaco, Consolas, monospace;
  font-size: 12px;
}
.row-alert {
  --el-table-tr-bg-color: #fff7ed;
}
.row-system {
  --el-table-tr-bg-color: #f1f5f9;
}

/* 暗夜模式下调整行高亮色，避免浅色背景刺眼 */
body.dark .row-alert {
  --el-table-tr-bg-color: var(--warning-bg);
}

body.dark .row-system {
  --el-table-tr-bg-color: var(--surface-hover);
}
</style>
