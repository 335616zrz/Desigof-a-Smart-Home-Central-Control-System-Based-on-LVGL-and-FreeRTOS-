<template>
  <view class="page">
    <view class="hero-card">
      <view class="hero-title">详细趋势</view>
      <view class="hero-subtitle">独立高精度趋势页，按时间范围细分并支持左右滑动查看全量走势。</view>
    </view>

    <view class="toolbar">
      <u-button size="mini" @click="pickDevice">{{ deviceLabel }}</u-button>
      <u-button size="mini" @click="pickRange">{{ rangeLabel }}</u-button>
      <u-button type="primary" size="mini" :loading="loading" @click="refresh">刷新</u-button>
    </view>

    <view class="meta-card">
      <view class="meta-item">
        <text class="meta-label">记录数</text>
        <text class="meta-value">{{ chartSeries.length }}</text>
      </view>
      <view class="meta-item">
        <text class="meta-label">设备数</text>
        <text class="meta-value">{{ deviceCount }}</text>
      </view>
      <view class="meta-item">
        <text class="meta-label">时间范围</text>
        <text class="meta-value">{{ rangeLabel }}</text>
      </view>
    </view>

    <view class="charts">
      <TelemetryEchartDetail
        title="温度详细趋势"
        :data="chartSeries"
        field="temperature"
        canvas-id="telemetryDetailTempChart"
        :x-axis-min="chartXAxisMin"
        :x-axis-max="chartXAxisMax"
        :interactive="true"
      />
      <TelemetryEchartDetail
        title="湿度详细趋势"
        :data="chartSeries"
        field="humidity"
        canvas-id="telemetryDetailHumidityChart"
        :x-axis-min="chartXAxisMin"
        :x-axis-max="chartXAxisMax"
        :interactive="true"
      />
    </view>

    <view class="hint-card">
      <text class="hint-title">说明</text>
      <text class="hint-text">这个页面与仪表盘分离，详细趋势使用和首页相同的稳定图表链路，但分桶更细、单屏展示更多点位。</text>
    </view>
  </view>
</template>

<script setup>
import { computed, ref } from 'vue'
import { onShow } from '@dcloudio/uni-app'
import TelemetryEchartDetail from '../../../components/TelemetryEchartDetail/TelemetryEchartDetail.vue'
import { useDeviceStore } from '../../../stores/device.js'
import { useTelemetryStore } from '../../../stores/telemetry.js'
import { toLocalIsoDateTime } from '../../../utils/format.js'

const deviceStore = useDeviceStore()
const telemetryStore = useTelemetryStore()

const selectedDeviceId = ref('')
const rangeId = ref('24h')
const loading = computed(() => Boolean(telemetryStore.loading))

const RANGE_ITEMS = [
  { id: '10m', name: '近10分钟', ms: 10 * 60 * 1000 },
  { id: '1h', name: '近1小时', ms: 60 * 60 * 1000 },
  { id: '24h', name: '近24小时', ms: 24 * 60 * 60 * 1000 },
  { id: '7d', name: '近7天', ms: 7 * 24 * 60 * 60 * 1000 },
  { id: '30d', name: '近30天', ms: 30 * 24 * 60 * 60 * 1000 },
  { id: 'all', name: '全部', ms: 0 }
]

const deviceLabel = computed(() => {
  if (!selectedDeviceId.value) return '全部设备'
  const device = (deviceStore.items || []).find((item) => item?.deviceId === selectedDeviceId.value)
  return device?.name || selectedDeviceId.value
})

const rangeLabel = computed(() => {
  return RANGE_ITEMS.find((item) => item.id === rangeId.value)?.name || '时间范围'
})

const chartSeries = computed(() => {
  const list = telemetryStore.parsedHistory || []
  const grouped = {}
  for (const item of list) {
    const deviceId = String(item?.deviceId ?? '')
    if (!deviceId) continue
    const ts = Number(item?.timestamp ?? 0)
    if (!Number.isFinite(ts) || ts <= 0) continue
    if (!grouped[deviceId]) grouped[deviceId] = []
    grouped[deviceId].push(item)
  }

  const out = []
  for (const deviceId of Object.keys(grouped).sort()) {
    const rows = grouped[deviceId].sort((a, b) => (a?.timestamp || 0) - (b?.timestamp || 0))
    for (const row of rows) out.push(row)
  }
  return out
})

const deviceCount = computed(() => {
  return new Set((chartSeries.value || []).map((item) => String(item?.deviceId || ''))).size
})

const chartXAxisMin = computed(() => {
  const range = RANGE_ITEMS.find((item) => item.id === rangeId.value)
  const ms = Number(range?.ms ?? 0)
  if (!ms) return 'dataMin'
  return Date.now() - ms
})

const chartXAxisMax = computed(() => Date.now())

function buildParams() {
  const now = Date.now()
  const range = RANGE_ITEMS.find((item) => item.id === rangeId.value)
  const ms = Number(range?.ms ?? 0)
  const fromTs = ms > 0 ? now - ms : 0

  return {
    deviceId: selectedDeviceId.value || undefined,
    from: toLocalIsoDateTime(fromTs),
    to: toLocalIsoDateTime(now),
    pageSize: 10000,
    page: 1
  }
}

async function refresh() {
  if (!deviceStore.items || deviceStore.items.length === 0) {
    await deviceStore.fetchList()
  }
  await telemetryStore.fetchHistory(buildParams(), { append: false })
}

function pickDevice() {
  const items = [{ id: '', name: '全部设备' }, ...(deviceStore.items || []).map((item) => ({
    id: item?.deviceId || '',
    name: item?.name || item?.deviceId || ''
  }))]

  uni.showActionSheet({
    itemList: items.map((item) => item.name),
    success: (res) => {
      const idx = res?.tapIndex ?? -1
      if (idx < 0) return
      selectedDeviceId.value = items[idx].id
      refresh()
    }
  })
}

function pickRange() {
  uni.showActionSheet({
    itemList: RANGE_ITEMS.map((item) => item.name),
    success: (res) => {
      const idx = res?.tapIndex ?? -1
      if (idx < 0) return
      rangeId.value = RANGE_ITEMS[idx].id
      refresh()
    }
  })
}

onShow(async () => {
  await refresh()
})
</script>

<style lang="scss" scoped>
.page {
  min-height: 100vh;
  padding: 12px;
  background:
    radial-gradient(circle at top right, rgba(255, 190, 92, 0.22), transparent 28%),
    linear-gradient(180deg, #fff8ef 0%, #f7f8fb 52%, #f3f5f9 100%);
}

.hero-card,
.meta-card,
.hint-card {
  background: rgba(255, 255, 255, 0.88);
  border: 1px solid rgba(255, 255, 255, 0.7);
  border-radius: 18px;
  box-shadow: 0 14px 40px rgba(15, 23, 42, 0.06);
  backdrop-filter: blur(10px);
}

.hero-card {
  padding: 16px;
}

.hero-title {
  font-size: 20px;
  font-weight: 700;
  color: #111827;
}

.hero-subtitle {
  margin-top: 8px;
  font-size: 12px;
  line-height: 1.6;
  color: #6b7280;
}

.toolbar {
  display: flex;
  flex-direction: row;
  flex-wrap: wrap;
  gap: 8px;
  margin-top: 12px;
}

.meta-card {
  display: flex;
  flex-direction: row;
  justify-content: space-between;
  gap: 10px;
  margin-top: 12px;
  padding: 12px 14px;
}

.meta-item {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.meta-label {
  font-size: 11px;
  color: #6b7280;
}

.meta-value {
  font-size: 15px;
  font-weight: 700;
  color: #111827;
}

.charts {
  margin-top: 12px;
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.hint-card {
  margin-top: 12px;
  padding: 14px;
}

.hint-title {
  display: block;
  font-size: 13px;
  font-weight: 600;
  color: #111827;
}

.hint-text {
  display: block;
  margin-top: 6px;
  font-size: 12px;
  line-height: 1.6;
  color: #6b7280;
}
</style>
