<template>
  <div class="page-shell page-padding">
    <section class="app-hero">
      <div class="app-hero__content">
        <p class="eyebrow">设备中心</p>
        <h1>设备总览</h1>
        <p class="lede">统一管理设备、监测状态并快速发现异常。</p>

        <div class="app-hero__badges">
          <el-tag effect="dark" type="info">总数 {{ total }}</el-tag>
          <el-tag effect="plain" :type="onlineRate >= 90 ? 'success' : 'warning'">在线率 {{ onlineRate }}%</el-tag>
          <span class="last-update muted">最近加载：{{ lastLoadedText }}</span>
        </div>

        <div class="app-hero__actions">
          <el-input
            v-model="keyword"
            placeholder="按名称/ID 搜索"
            clearable
            class="search-input"
            @keyup.enter="load"
            @clear="load"
          />
          <el-button type="primary" @click="openCreate">新建设备</el-button>
          <el-button plain @click="load">刷新</el-button>
        </div>
      </div>

      <div class="app-hero__panel">
        <div class="app-mini-grid">
          <div class="app-mini-card">
            <div class="app-chip-icon info"><el-icon><Cpu /></el-icon></div>
            <div class="mini-meta">
              <p>设备总数</p>
              <strong>{{ total }}</strong>
              <span>已登记</span>
            </div>
          </div>
          <div class="app-mini-card">
            <div class="app-chip-icon success"><el-icon><Connection /></el-icon></div>
            <div class="mini-meta">
              <p>在线设备</p>
              <strong>{{ online }}</strong>
              <span>实时在线</span>
            </div>
          </div>
          <div class="app-mini-card">
            <div class="app-chip-icon warning"><el-icon><WarningFilled /></el-icon></div>
            <div class="mini-meta">
              <p>离线设备</p>
              <strong>{{ offline }}</strong>
              <span>待排查</span>
            </div>
          </div>
        </div>
      </div>
    </section>

    <el-card shadow="never">
      <template #header>
        <div class="flex-between" style="align-items:center;">
          <span>设备列表</span>
          <div style="display:flex; gap:8px; align-items:center;">
            <el-button text @click="load">刷新</el-button>
          </div>
        </div>
      </template>

      <TableSkeleton v-if="store.loading && store.items.length === 0" :rows="8" />
      <el-table
        v-else
        :data="store.items"
        :loading="store.loading"
        border
        style="width: 100%"
        stripe
      >
        <el-table-column prop="deviceId" label="设备ID" width="160" />
        <el-table-column prop="name" label="名称" min-width="120" />
        <el-table-column label="状态" width="100">
          <template #default="{ row }">
            <StatusBadge :status="row.status" />
          </template>
        </el-table-column>
        <el-table-column prop="firmwareVersion" label="固件" width="120" class-name="mobile-hide" />
        <el-table-column prop="lastSeenAt" label="最近在线" :formatter="fmt" min-width="150" class-name="mobile-hide" />
        <el-table-column label="操作" width="180" fixed="right">
          <template #default="{ row }">
            <el-button link type="primary" size="small" @click="view(row.deviceId)">详情</el-button>
            <el-button link size="small" @click="edit(row)">编辑</el-button>
            <el-popconfirm title="确认删除该设备？" @confirm="remove(row.deviceId)">
              <template #reference>
                <el-button link type="danger" size="small">删除</el-button>
              </template>
            </el-popconfirm>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <DeviceFormDialog v-model="formVisible" :device="editing" @submitted="load" />
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, computed } from 'vue';
import { useRouter } from 'vue-router';
import { Connection, Cpu, WarningFilled } from '@element-plus/icons-vue';
import { useDeviceStore } from '@/stores/device';
import StatusBadge from '@/components/StatusBadge.vue';
import TableSkeleton from '@/components/TableSkeleton.vue';
import DeviceFormDialog from './DeviceFormDialog.vue';
import { formatDateTime } from '@/utils/format';

const store = useDeviceStore();
const router = useRouter();

const keyword = ref('');
const formVisible = ref(false);
const editing = ref();
const lastLoaded = ref<number | null>(null);

const total = computed(() => store.items.length);
const online = computed(() => store.items.filter(d => d.status === 'ONLINE').length);
const offline = computed(() => store.items.filter(d => d.status === 'OFFLINE').length);
const onlineRate = computed(() => {
  if (!total.value) return 0;
  return Math.round((online.value / total.value) * 100);
});
const lastLoadedText = computed(() => (lastLoaded.value ? formatDateTime(lastLoaded.value) : '尚未加载'));

function fmt(_: unknown, __: unknown, value: string) {
  return formatDateTime(value);
}

async function load() {
  await store.fetchList({ keyword: keyword.value, pageSize: 200 });
  lastLoaded.value = Date.now();
}

function view(deviceId: string) {
  router.push(`/devices/${deviceId}`);
}

function openCreate() {
  editing.value = undefined;
  formVisible.value = true;
}

function edit(device: any) {
  editing.value = device;
  formVisible.value = true;
}

async function remove(id: string) {
  await store.remove(id);
}

onMounted(load);
</script>

<style scoped>
.search-input {
  width: 280px;
}

@media (max-width: 768px) {
  .search-input {
    width: 100%;
  }

  .app-hero__actions {
    display: flex;
    flex-direction: column;
    gap: 10px;
    width: 100%;
  }

  .app-hero__actions .el-button {
    width: 100%;
  }
}
</style>
