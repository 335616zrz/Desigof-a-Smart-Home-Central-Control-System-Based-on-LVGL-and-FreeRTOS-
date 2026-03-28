<template>
  <view class="chart-card" :class="toneClass">
    <view class="header">
      <view class="header-main">
        <view class="title">{{ title }}</view>
        <text v-if="detailHint" class="detail-hint">{{ detailHint }}</text>
      </view>
      <view v-if="latestValue !== null" class="latest">
        <text class="value">{{ latestValue }}</text>
        <text class="unit">{{ unit }}</text>
      </view>
    </view>

    <view v-if="hasData" class="chart-wrap">
      <mrsong-charts
        type="line"
        title=""
        :chartsData="chartsData"
        :options="chartOptions"
        :config="chartConfig"
        :ontouch="interactive"
      />
    </view>

    <view v-if="showLegend" class="legend">
      <view v-for="item in legendItems" :key="item.name" class="legend-item">
        <view class="legend-dot" :style="{ backgroundColor: item.color }"></view>
        <text class="legend-text">{{ item.name }}</text>
      </view>
    </view>

    <view v-if="!hasData" class="empty">
      <text class="empty-text">暂无数据</text>
    </view>
  </view>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  title: { type: String, default: '详细趋势' },
  data: { type: Array, default: () => [] },
  field: { type: String, default: 'temperature' },
  canvasId: { type: String, default: 'telemetryDetailChart' },
  forceUseOldCanvas: { type: Boolean, default: false },
  xAxisMin: { type: [Number, String], default: 'dataMin' },
  xAxisMax: { type: [Number, String], default: 'dataMax' },
  interactive: { type: Boolean, default: true }
})

const unit = computed(() => (props.field === 'humidity' ? '%' : '°C'))
const toneClass = computed(() => (props.field === 'humidity' ? 'humidity' : 'temperature'))

const palette = computed(() => {
  return props.field === 'humidity'
    ? ['#00a86b', '#24bf7a', '#55cf94', '#7ee0ae', '#1b8f64', '#4bbf9b']
    : ['#ff8a3d', '#ff6b2c', '#ffb24d', '#ff9f68', '#ff7f50', '#f97316']
})

function formatHHmm(ts) {
  const d = new Date(ts)
  const hh = String(d.getHours()).padStart(2, '0')
  const mm = String(d.getMinutes()).padStart(2, '0')
  return `${hh}:${mm}`
}

function formatHHmmss(ts) {
  const d = new Date(ts)
  const hh = String(d.getHours()).padStart(2, '0')
  const mm = String(d.getMinutes()).padStart(2, '0')
  const ss = String(d.getSeconds()).padStart(2, '0')
  return `${hh}:${mm}:${ss}`
}

function formatMMDDHHmm(ts) {
  const d = new Date(ts)
  const mm = String(d.getMonth() + 1).padStart(2, '0')
  const dd = String(d.getDate()).padStart(2, '0')
  const hh = String(d.getHours()).padStart(2, '0')
  const min = String(d.getMinutes()).padStart(2, '0')
  return `${mm}-${dd} ${hh}:${min}`
}

function formatLabel(ts, rangeMs) {
  if (rangeMs <= 15 * 60 * 1000) return formatHHmmss(ts)
  if (rangeMs <= 24 * 60 * 60 * 1000) return formatHHmm(ts)
  return formatMMDDHHmm(ts)
}

function resolveBucketCount(rangeMs) {
  if (rangeMs <= 15 * 60 * 1000) return 120
  if (rangeMs <= 60 * 60 * 1000) return 180
  if (rangeMs <= 6 * 60 * 60 * 1000) return 240
  if (rangeMs <= 24 * 60 * 60 * 1000) return 288
  if (rangeMs <= 7 * 24 * 60 * 60 * 1000) return 240
  return 180
}

const normalizedPoints = computed(() => {
  const out = []
  for (const item of props.data || []) {
    const deviceId = String(item?.deviceId ?? '')
    if (!deviceId) continue

    const ts = Number(item?.timestamp ?? 0)
    if (!Number.isFinite(ts) || ts <= 0) continue

    const value = Number(item?.[props.field])
    if (!Number.isFinite(value)) continue

    out.push({ deviceId, timestamp: ts, value })
  }

  out.sort((a, b) => (a?.timestamp || 0) - (b?.timestamp || 0))
  return out
})

const hasData = computed(() => normalizedPoints.value.length > 0)

const latestValue = computed(() => {
  const list = normalizedPoints.value
  if (!list.length) return null
  const latest = list[list.length - 1]
  return Number(latest?.value).toFixed(2)
})

const chartModel = computed(() => {
  const points = normalizedPoints.value
  if (!points.length) {
    return {
      categories: [],
      series: [],
      legendItems: [],
      enableScroll: false,
      bucketCount: 0,
      pointCount: 0
    }
  }

  const grouped = new Map()
  let dataMin = points[0].timestamp
  let dataMax = points[points.length - 1].timestamp

  for (const point of points) {
    const deviceId = point.deviceId
    if (!grouped.has(deviceId)) grouped.set(deviceId, [])
    grouped.get(deviceId).push(point)
  }

  const minProp = typeof props.xAxisMin === 'number' ? Number(props.xAxisMin) : NaN
  const maxProp = typeof props.xAxisMax === 'number' ? Number(props.xAxisMax) : NaN
  const effectiveMin = Number.isFinite(minProp) ? Math.min(minProp, dataMin) : dataMin
  const effectiveMax = Number.isFinite(maxProp) ? Math.max(maxProp, dataMax) : dataMax
  const rangeMs = Math.max(effectiveMax - effectiveMin, 0)
  const bucketCount = Math.max(24, resolveBucketCount(rangeMs))
  const bucketMs = rangeMs > 0 ? Math.max(Math.ceil(rangeMs / Math.max(bucketCount - 1, 1)), 1000) : 1000

  const bucketTimestamps = []
  for (let i = 0; i < bucketCount; i += 1) {
    bucketTimestamps.push(effectiveMin + i * bucketMs)
  }
  if (bucketTimestamps.length && bucketTimestamps[bucketTimestamps.length - 1] < effectiveMax) {
    bucketTimestamps.push(effectiveMax)
  }

  const keys = Array.from(grouped.keys()).sort()
  const legendItems = []
  const series = []

  for (let i = 0; i < keys.length; i += 1) {
    const deviceId = keys[i]
    const color = palette.value[i % palette.value.length]
    const values = bucketTimestamps.map(() => null)
    const items = grouped.get(deviceId) || []

    for (const item of items) {
      const ts = Number(item?.timestamp || 0)
      let index = 0
      if (bucketTimestamps.length > 1) {
        index = Math.round((ts - effectiveMin) / bucketMs)
      }
      if (index < 0) index = 0
      if (index >= bucketTimestamps.length) index = bucketTimestamps.length - 1
      values[index] = Number(item?.value)
    }

    legendItems.push({ name: deviceId, color })
    series.push({
      name: deviceId,
      data: values
    })
  }

  return {
    categories: bucketTimestamps.map((ts) => formatLabel(ts, rangeMs)),
    series,
    legendItems,
    enableScroll: Boolean(props.interactive || bucketTimestamps.length > 10),
    bucketCount: bucketTimestamps.length,
    pointCount: points.length
  }
})

const chartsData = computed(() => ({
  categories: chartModel.value.categories,
  series: chartModel.value.series
}))

const visibleItemCount = computed(() => {
  const count = chartModel.value.categories.length
  if (count >= 180) return 12
  if (count >= 90) return 10
  return 8
})

const chartConfig = computed(() => ({
  enableScroll: chartModel.value.enableScroll,
  scrollShow: chartModel.value.enableScroll,
  scrollAlign: 'left',
  itemCount: visibleItemCount.value,
  rotateLabel: chartModel.value.categories.length > 8,
  color: palette.value
}))

const chartOptions = computed(() => ({
  color: palette.value,
  animation: false,
  dataLabel: false,
  dataPointShape: false,
  enableScroll: chartModel.value.enableScroll,
  xAxis: {
    itemCount: visibleItemCount.value,
    scrollShow: chartModel.value.enableScroll,
    scrollAlign: 'left',
    rotateLabel: chartModel.value.categories.length > 8,
    fontSize: 10,
    margin: 8
  },
  yAxis: {
    gridType: 'dash',
    dashLength: 2,
    data: [{
      type: 'value',
      unit: unit.value,
      fontSize: 11,
      tofix: 2
    }]
  },
  legend: {
    show: false
  },
  extra: {
    line: {
      type: 'curve',
      width: 2
    }
  }
}))

const legendItems = computed(() => chartModel.value.legendItems || [])
const showLegend = computed(() => legendItems.value.length > 1)

const detailHint = computed(() => {
  if (!chartModel.value.pointCount) return ''
  const parts = [
    `原始${chartModel.value.pointCount}条`,
    `展示${chartModel.value.bucketCount}段`
  ]
  if (chartModel.value.enableScroll) parts.push('可左右滑动')
  return parts.join(' · ')
})
</script>

<style lang="scss" scoped>
.chart-card {
  background: #ffffff;
  border-radius: 16px;
  padding: 14px;
  box-shadow: 0 10px 30px rgba(15, 23, 42, 0.06);
}

.header {
  display: flex;
  flex-direction: row;
  align-items: flex-start;
  justify-content: space-between;
  gap: 10px;
  margin-bottom: 10px;
}

.header-main {
  display: flex;
  flex-direction: column;
  gap: 4px;
  min-width: 0;
}

.title {
  font-size: 15px;
  font-weight: 600;
  color: #111827;
}

.detail-hint {
  font-size: 11px;
  line-height: 1.5;
  color: #6b7280;
}

.latest {
  display: flex;
  flex-direction: row;
  align-items: baseline;
  gap: 2px;
  flex-shrink: 0;
}

.value {
  font-size: 18px;
  font-weight: 700;
  color: #f97316;
}

.chart-card.humidity .value {
  color: #00a86b;
}

.unit {
  font-size: 12px;
  color: #6b7280;
}

.chart-wrap {
  width: 100%;
  min-height: 300px;
}

.legend {
  width: 100%;
  display: flex;
  flex-direction: row;
  flex-wrap: wrap;
  gap: 10px;
  padding-top: 8px;
}

.legend-item {
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 6px;
}

.legend-dot {
  width: 8px;
  height: 8px;
  border-radius: 999px;
  flex-shrink: 0;
}

.legend-text {
  font-size: 11px;
  color: #6b7280;
}

.empty {
  width: 100%;
  min-height: 300px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.empty-text {
  font-size: 12px;
  color: #6b7280;
}
</style>
