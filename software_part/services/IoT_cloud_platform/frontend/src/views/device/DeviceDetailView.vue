<template>
  <div class="page-shell page-padding" v-if="device">
    <section class="app-hero">
      <div class="app-hero__content">
        <p class="eyebrow">设备详情</p>
        <h1>{{ device.name }}</h1>
        <p class="lede">设备ID：{{ device.deviceId }}</p>

        <div class="app-hero__badges">
          <el-tag :type="statusTag" effect="dark">{{ statusLabel }}</el-tag>
          <el-tag effect="plain" type="info">固件 {{ device.firmwareVersion || '未设置' }}</el-tag>
          <span class="muted">最近在线：{{ device.lastSeenAt ? fmt(null, null, device.lastSeenAt) : '未知' }}</span>
        </div>

        <div class="app-hero__actions">
          <el-button @click="provision">生成凭证</el-button>
          <el-button type="primary" @click="openEdit">编辑</el-button>
          <el-button plain @click="reload">刷新</el-button>
        </div>
      </div>

      <div class="app-hero__panel">
        <div class="app-mini-grid">
          <div class="app-mini-card">
            <div class="app-chip-icon info"><el-icon><Connection /></el-icon></div>
            <div class="mini-meta">
              <p>状态</p>
              <strong>{{ statusLabel }}</strong>
              <span>{{ device.status }}</span>
            </div>
          </div>
          <div class="app-mini-card">
            <div class="app-chip-icon success"><el-icon><Timer /></el-icon></div>
            <div class="mini-meta">
              <p>最近遥测</p>
              <strong>{{ recent.length }}</strong>
              <span>缓存记录</span>
            </div>
          </div>
          <div class="app-mini-card">
            <div class="app-chip-icon warning"><el-icon><BellFilled /></el-icon></div>
            <div class="mini-meta">
              <p>历史查看</p>
              <strong>跳转</strong>
              <span>查看全量记录</span>
            </div>
          </div>
        </div>
      </div>
    </section>

    <el-row :gutter="16">
      <el-col :xs="24" :md="12">
        <el-card shadow="never">
          <template #header><span>设备信息</span></template>
          <div style="display:flex; flex-direction:column; gap:6px;">
            <div><span class="muted">状态</span><div><StatusBadge :status="device.status" /></div></div>
            <div class="muted">固件版本：{{ device.firmwareVersion || '-' }}</div>
            <div class="muted">最近在线：{{ device.lastSeenAt || '-' }}</div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :md="12">
        <el-card shadow="never">
          <template #header><span>灯光控制</span></template>
          <div style="display:flex; flex-direction:column; gap:10px;">
            <div style="display:flex; align-items:center; justify-content:space-between; gap:12px;">
              <div style="display:flex; flex-direction:column; gap:4px;">
                <strong>夜灯（白光）</strong>
                <span class="muted">通过 MQTT 下发；设备回执后自动同步</span>
              </div>
              <el-switch
                v-model="nightLightOn"
                :disabled="nightLightLoading"
                @change="onNightLightChange"
              />
            </div>

            <div v-if="device" style="display:flex; flex-wrap:wrap; gap:8px;">
              <el-tag size="small" :type="device.status === 'ONLINE' ? 'success' : device.status === 'OFFLINE' ? 'warning' : 'info'" effect="plain">
                设备：{{ statusLabel }}
              </el-tag>
              <el-tag size="small" :type="stream.connected ? 'success' : 'warning'" effect="plain">
                实时通道：{{ stream.connected ? '已连接' : '未连接' }}
              </el-tag>
              <el-tag v-if="shadow" size="small" :type="shadow.nightLightPending ? 'warning' : 'success'" effect="plain">
                {{ shadow.nightLightPending ? '等待设备回执' : '已同步' }}
              </el-tag>
              <el-tag v-if="shadow" size="small" type="info" effect="plain">
                云端期望：{{ boolLabel(shadow.desiredNightLightOn) }}
              </el-tag>
              <el-tag v-if="shadow" size="small" type="info" effect="plain">
                设备回执：{{ boolLabel(shadow.reportedNightLightOn) }}
              </el-tag>
              <el-tag v-if="shadow && shadow.nightLightSource" size="small" type="info" effect="plain">
                来源：{{ shadowSourceLabel(shadow.nightLightSource) }}
              </el-tag>
            </div>

            <div v-if="shadowErr" class="muted" style="color: var(--el-color-danger);">
              {{ shadowErr }}
            </div>

            <div v-if="device?.status === 'OFFLINE'" class="muted">
              设备当前离线：指令将保留，设备上线后自动生效。
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <el-card shadow="never" style="margin-top: 16px;">
      <template #header>
        <div class="flex-between">
          <span>最近遥测</span>
          <el-button link @click="goHistory">查看历史</el-button>
        </div>
      </template>
      <el-table :data="recent" size="small">
        <el-table-column prop="timestamp" label="时间" :formatter="fmt" />
        <el-table-column prop="temperature" label="温度" />
        <el-table-column prop="humidity" label="湿度" />
        <el-table-column prop="pressure" label="气压" />
      </el-table>
    </el-card>

    <DeviceProvisionDialog v-model="provisionVisible" :credential="store.credential" />
    <DeviceFormDialog v-model="formVisible" :device="device" @submitted="reload" />
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, onUnmounted, ref, watch } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { BellFilled, Connection, Timer } from '@element-plus/icons-vue';
import { ElMessage } from 'element-plus';
import { useDeviceStore } from '@/stores/device';
import StatusBadge from '@/components/StatusBadge.vue';
import DeviceProvisionDialog from '@/components/DeviceProvisionDialog.vue';
import DeviceFormDialog from './DeviceFormDialog.vue';
import { useTelemetryStream } from '@/stores/telemetryStream';
import { formatDateTime } from '@/utils/format';
import { getDeviceShadow, setNightLight, type DeviceShadowView } from '@/api/device';
import { useAppStore } from '@/stores/app';
import { initMqtt } from '@/services/mqtt';

const route = useRoute();
const router = useRouter();
const store = useDeviceStore();
const stream = useTelemetryStream();
const appStore = useAppStore();

const provisionVisible = ref(false);
const formVisible = ref(false);

const deviceId = computed(() => route.params.id as string);
const device = computed(() => store.current);
const recent = computed(() => stream.points.filter(p => p.deviceId === deviceId.value).slice(-50).reverse());
const statusLabel = computed(() => {
  if (!device.value) return '';
  if (device.value.status === 'ONLINE') return '在线';
  if (device.value.status === 'OFFLINE') return '离线';
  return device.value.status || '未知';
});
const statusTag = computed(() => (device.value?.status === 'ONLINE' ? 'success' : device.value?.status === 'OFFLINE' ? 'warning' : 'info'));

const nightLightOn = ref(false);
const nightLightLoading = ref(false);
const shadow = ref<DeviceShadowView | null>(null);
const shadowLoading = ref(false);
const shadowErr = ref<string | null>(null);

let shadowTimer: number | null = null;
let deviceTimer: number | null = null;

function boolLabel(v: unknown) {
  if (v === null || typeof v === 'undefined') return '未知';
  return Boolean(v) ? '开' : '关';
}

function shadowSourceLabel(src: string) {
  if (src === 'reported') return '设备回执';
  if (src === 'desired') return '云端期望';
  if (src === 'default') return '默认';
  return src || '-';
}

function fmt(_: unknown, __: unknown, value: unknown) {
  if (typeof value === 'number') return formatDateTime(value);
  if (typeof value === 'string') {
    const ts = Date.parse(value);
    return Number.isFinite(ts) ? formatDateTime(ts) : value;
  }
  return '-';
}

async function loadShadow() {
  if (!deviceId.value) return;
  if (shadowLoading.value) return;
  shadowLoading.value = true;
  shadowErr.value = null;
  try {
    const data = await getDeviceShadow(deviceId.value);
    shadow.value = data;

    // Switch shows desired state when available; otherwise fall back to effective state.
    if (!nightLightLoading.value) {
      const desired = (data as any)?.desiredNightLightOn;
      if (desired !== null && typeof desired !== 'undefined') {
        nightLightOn.value = Boolean(desired);
      } else if (typeof (data as any)?.nightLightOn !== 'undefined') {
        nightLightOn.value = Boolean((data as any).nightLightOn);
      }
    }
  } catch (e: any) {
    shadowErr.value = e?.response?.data?.message || e?.message || '获取设备状态失败';
  } finally {
    shadowLoading.value = false;
  }
}

async function reload() {
  await store.fetchOne(deviceId.value);
  await loadShadow();
}

async function provision() {
  await store.provision(deviceId.value);
  provisionVisible.value = true;
}

function openEdit() {
  formVisible.value = true;
}

function goHistory() {
  router.push({ name: 'telemetry-history', query: { deviceId: deviceId.value } });
}

async function onNightLightChange(next: boolean | string | number) {
  const on = Boolean(next);
  if (!device.value) return;
  if (nightLightLoading.value) return;
  nightLightLoading.value = true;
  try {
    await setNightLight(deviceId.value, on);
    ElMessage.success(on ? '夜灯已开启' : '夜灯已关闭');
    // Refresh shadow quickly to show pending/synced state.
    loadShadow();
  } catch (e: any) {
    nightLightOn.value = !on;
    ElMessage.error(e?.message || '夜灯控制失败');
  } finally {
    nightLightLoading.value = false;
  }
}

function startPolling() {
  // Shadow is the source of truth for multi-user consistency and pending state.
  shadowTimer = window.setInterval(() => {
    if (document.hidden) return;
    loadShadow();
  }, 1500);
  // Device status changes less frequently; refresh at a lower rate.
  deviceTimer = window.setInterval(() => {
    if (document.hidden) return;
    if (!deviceId.value) return;
    store.fetchOne(deviceId.value);
  }, 5000);
}

function stopPolling() {
  if (shadowTimer) window.clearInterval(shadowTimer);
  if (deviceTimer) window.clearInterval(deviceTimer);
  shadowTimer = null;
  deviceTimer = null;
}

onMounted(() => {
  // Ensure MQTT realtime channel state is visible even if user doesn't visit dashboard first.
  initMqtt({ host: appStore.mqttWsUrl });
  reload();
  startPolling();
});

onUnmounted(stopPolling);

watch(deviceId, () => {
  shadow.value = null;
  shadowErr.value = null;
  reload();
});
</script>
