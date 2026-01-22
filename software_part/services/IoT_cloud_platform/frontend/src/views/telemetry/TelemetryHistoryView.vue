<template>
  <div class="page-shell page-padding">
    <section class="app-hero">
      <div class="app-hero__content">
        <p class="eyebrow">遥测历史</p>
        <h1>全量数据 · 快速筛查</h1>
        <p class="lede">支持按设备、指标、时间范围查询并导出。</p>

        <div class="app-hero__badges">
          <el-tag effect="dark" type="info">记录 {{ telemetry.total }}</el-tag>
          <el-tag effect="plain" type="success">设备 {{ deviceOptions.length }}</el-tag>
          <span class="last-update">查询区间：{{ rangeText }}</span>
        </div>

        <div class="app-hero__actions">
          <el-dropdown @command="handleExport">
            <el-button type="success" :disabled="telemetry.history.length === 0">
              导出数据<el-icon class="el-icon--right"><ArrowDown /></el-icon>
            </el-button>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item command="excel">导出为 Excel</el-dropdown-item>
                <el-dropdown-item command="csv">导出为 CSV</el-dropdown-item>
                <el-dropdown-item command="json">导出为 JSON</el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
          <el-button type="primary" @click="load">刷新</el-button>
        </div>
      </div>

      <div class="app-hero__panel">
        <div class="app-mini-grid">
          <div class="app-mini-card">
            <div class="app-chip-icon info"><el-icon><TrendCharts /></el-icon></div>
            <div class="mini-meta">
              <p>温度/湿度/气压</p>
              <strong>三指标</strong>
              <span>默认展示</span>
            </div>
          </div>
          <div class="app-mini-card">
            <div class="app-chip-icon success"><el-icon><Connection /></el-icon></div>
            <div class="mini-meta">
              <p>设备选项</p>
              <strong>{{ deviceOptions.length }}</strong>
              <span>可筛选</span>
            </div>
          </div>
          <div class="app-mini-card">
            <div class="app-chip-icon warning"><el-icon><Histogram /></el-icon></div>
            <div class="mini-meta">
              <p>分页</p>
              <strong>{{ filters.pageSize }}</strong>
              <span>每页记录</span>
            </div>
          </div>
        </div>
      </div>
    </section>

    <el-card shadow="never">
      <div class="app-section-card" style="margin-bottom: 12px;">
        <el-form :inline="true" :model="filters" class="filter">
          <el-form-item label="设备">
            <el-select
              v-model="filters.deviceId"
              placeholder="选择设备"
              filterable
              clearable
              style="width: 280px"
            >
              <el-option
                v-for="device in deviceOptions"
                :key="device.value"
                :label="device.label"
                :value="device.value"
              />
            </el-select>
          </el-form-item>
          <el-form-item label="指标">
            <el-select v-model="filters.metric" clearable style="width: 140px">
              <el-option label="温度" value="temperature" />
              <el-option label="湿度" value="humidity" />
              <el-option label="气压" value="pressure" />
            </el-select>
          </el-form-item>
          <el-form-item label="开始时间">
            <el-date-picker v-model="filters.from" type="datetime" placeholder="开始" />
          </el-form-item>
          <el-form-item label="结束时间">
            <el-date-picker v-model="filters.to" type="datetime" placeholder="结束" />
          </el-form-item>
          <el-form-item>
            <el-button type="primary" @click="load">查询</el-button>
          </el-form-item>
        </el-form>
      </div>

      <el-table
        :data="telemetry.history"
        stripe
        :loading="telemetry.loading"
        height="calc(100vh - 380px)"
        style="margin-top: 10px;"
      >
        <el-table-column prop="deviceId" label="设备" width="160" fixed />
        <el-table-column prop="temperature" label="温度 (°C)" width="120" />
        <el-table-column prop="humidity" label="湿度 (%)" width="120" />
        <el-table-column prop="pressure" label="气压 (hPa)" width="120" />
        <el-table-column prop="timestamp" label="时间" :formatter="format" min-width="180" />
      </el-table>

      <el-pagination
        v-if="telemetry.total > filters.pageSize"
        v-model:current-page="filters.page"
        v-model:page-size="filters.pageSize"
        :page-sizes="[50, 100, 200, 500]"
        :total="telemetry.total"
        layout="total, sizes, prev, pager, next, jumper"
        style="margin-top: 16px; justify-content: center;"
        @size-change="load"
        @current-change="load"
      />
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { reactive, onMounted, computed } from 'vue';
import { useRoute } from 'vue-router';
import { ArrowDown, Connection, Histogram, TrendCharts } from '@element-plus/icons-vue';
import { ElMessage } from 'element-plus';
import { useTelemetryStore } from '@/stores/telemetry';
import { useDeviceStore } from '@/stores/device';
import { formatDateTime, toLocalIsoDateTime } from '@/utils/format';
import { exportTelemetryToExcel, exportTelemetryToCSV, exportToJSON } from '@/utils/export';

const telemetry = useTelemetryStore();
const deviceStore = useDeviceStore();
const route = useRoute();

const filters = reactive({
  deviceId: (route.query.deviceId as string) || '',
  metric: '',
  from: new Date(Date.now() - 3600 * 1000),
  to: new Date(),
  page: 1,
  pageSize: 100
});

const deviceOptions = computed(() =>
  deviceStore.items.map(d => ({ label: `${d.name} (${d.deviceId})`, value: d.deviceId }))
);

const rangeText = computed(() => {
  if (!filters.from || !filters.to) return '未选择时间范围';
  return `${formatDateTime(filters.from as unknown as number)} - ${formatDateTime(filters.to as unknown as number)}`;
});

onMounted(async () => {
  await deviceStore.fetchList({ pageSize: 200 });
  await load();
});

async function load() {
  const params: any = {
    deviceId: filters.deviceId,
    pageSize: filters.pageSize,
    page: filters.page
  };

  if (filters.metric) {
    params.metric = filters.metric;
  }

  if (filters.from && filters.to) {
    params.from = toLocalIsoDateTime(filters.from);
    params.to = toLocalIsoDateTime(filters.to);
  }

  await telemetry.loadHistory(params);
}

const format = (_: unknown, __: unknown, value: number) => formatDateTime(value);

function handleExport(command: string) {
  if (telemetry.history.length === 0) {
    ElMessage.warning('没有数据可导出');
    return;
  }

  try {
    switch (command) {
      case 'excel':
        exportTelemetryToExcel(telemetry.history);
        ElMessage.success('Excel 文件已下载');
        break;
      case 'csv':
        exportTelemetryToCSV(telemetry.history);
        ElMessage.success('CSV 文件已下载');
        break;
      case 'json':
        exportToJSON(telemetry.history);
        ElMessage.success('JSON 文件已下载');
        break;
    }
  } catch (error) {
    ElMessage.error('导出失败: ' + (error as Error).message);
  }
}
</script>

<style scoped>
.filter {
  padding: 4px 0;
}
</style>
