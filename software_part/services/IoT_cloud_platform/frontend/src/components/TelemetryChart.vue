<template>
  <el-card shadow="hover" :body-style="{ padding: '12px 12px 4px' }">
    <div class="chart-title">{{ title }}</div>
    <div ref="chartRef" class="chart"></div>
  </el-card>
</template>

<script setup lang="ts">
import { onMounted, onUnmounted, ref, watch, shallowRef } from 'vue';
import { useDebounceFn } from '@vueuse/core';
import type { TelemetryRecord } from '@/types/models';
import { useAppStore } from '@/stores/app';
import { useAlertRuleStore } from '@/stores/alertRule';

const props = withDefaults(defineProps<{
  title: string;
  dataSeries: TelemetryRecord[];
  metric: keyof TelemetryRecord;
  showThresholds?: boolean;
  xAxisMin?: number | string;
  xAxisMax?: number | string;
}>(), {
  showThresholds: false
});

const chartRef = ref<HTMLDivElement>();
const chartInstance = shallowRef<any>(null);
let echarts: typeof import('echarts/core') | null = null;
let registered = false;

const appStore = useAppStore();
const ruleStore = useAlertRuleStore();

function getCssVar(name: string, fallback: string) {
  if (typeof window === 'undefined') return fallback;
  const value = getComputedStyle(document.documentElement).getPropertyValue(name).trim();
  return value || fallback;
}

function getThemeColors() {
  return {
    textPrimary: getCssVar('--text-primary', '#111827'),
    textSecondary: getCssVar('--text-secondary', '#6b7280'),
    textTertiary: getCssVar('--text-tertiary', '#9ca3af'),
    borderPrimary: getCssVar('--border-primary', '#e5e7eb'),
    surface: getCssVar('--surface', '#ffffff'),
    surfaceHover: getCssVar('--surface-hover', '#f9fafb'),
    bgSecondary: getCssVar('--bg-secondary', '#f9fafb')
  };
}

async function ensureEcharts() {
  if (echarts) return echarts;
  const core = await import('echarts/core');
  if (!registered) {
    const { LineChart } = await import('echarts/charts');
    const { GridComponent, TooltipComponent, LegendComponent, TitleComponent, DataZoomComponent } = await import('echarts/components');
    const { CanvasRenderer } = await import('echarts/renderers');
    core.use([LineChart, GridComponent, TooltipComponent, LegendComponent, TitleComponent, DataZoomComponent, CanvasRenderer]);
    registered = true;
  }
  echarts = core;
  return core;
}

// 数据采样：当有效数据点过多时进行采样（保留 null 断点，避免把断线“采样没了”）
function sampleData(data: [number, number | null][], maxPoints = 500): [number, number | null][] {
  const validCount = data.reduce((count, [, value]) => (value == null ? count : count + 1), 0);
  if (validCount <= maxPoints) return data;
  const step = Math.ceil(validCount / maxPoints);

  let validIndex = 0;
  const lastValidIndex = validCount - 1;
  return data.filter(([, value]) => {
    if (value == null) return true;
    const keep = validIndex === 0 || validIndex === lastValidIndex || validIndex % step === 0;
    validIndex++;
    return keep;
  });
}

function median(values: number[]) {
  if (!values.length) return null;
  const sorted = [...values].sort((a, b) => a - b);
  const mid = Math.floor(sorted.length / 2);
  return sorted.length % 2 ? sorted[mid] : (sorted[mid - 1] + sorted[mid]) / 2;
}

function computeGapBreakMs(sortedTimestamps: number[]) {
  const diffs: number[] = [];
  for (let i = 1; i < sortedTimestamps.length; i++) {
    const diff = sortedTimestamps[i] - sortedTimestamps[i - 1];
    // 只用“正常上报间隔”估算（过滤掉小时级别的大空洞）
    if (diff > 0 && diff <= 5 * 60 * 1000) diffs.push(diff);
  }
  const typical = median(diffs);
  // 默认最小断线阈值 20s；如果设备上报周期更长，则按周期倍数放宽
  return Math.max((typical ?? 5000) * 4, 20 * 1000);
}

function insertGapBreaks(data: [number, number][]) {
  if (data.length < 2) return data as unknown as [number, number | null][];
  const timestamps = data.map(([t]) => t);
  const gapBreakMs = computeGapBreakMs(timestamps);

  const result: [number, number | null][] = [];
  let prevTs: number | null = null;
  for (const [ts, value] of data) {
    if (prevTs != null && ts - prevTs > gapBreakMs) {
      // 插入一个 null 点作为断点，避免长时间未上报时折线跨越连接
      result.push([prevTs + 1, null]);
    }
    result.push([ts, value]);
    prevTs = ts;
  }
  return result;
}

function formatHHmm(ts: number) {
  const d = new Date(ts);
  const hh = String(d.getHours()).padStart(2, '0');
  const mm = String(d.getMinutes()).padStart(2, '0');
  return `${hh}:${mm}`;
}

function parseTimeValue(value: unknown) {
  if (typeof value === 'number') return value;
  if (typeof value === 'string') {
    const asNumber = Number(value);
    if (Number.isFinite(asNumber)) return asNumber;
    return new Date(value).getTime();
  }
  return NaN;
}

function timeAxisLabelFormatter(value: unknown) {
  const ts = parseTimeValue(value);
  if (!Number.isFinite(ts)) return '';
  return formatHHmm(ts);
}

function getTimeAxisTicks() {
  const min = props.xAxisMin;
  const max = props.xAxisMax;
  const minNumber = typeof min === 'number' ? min : null;
  const maxNumber = typeof max === 'number' ? max : null;
  if (minNumber == null || maxNumber == null) return { min, max };

  const range = Math.max(maxNumber - minNumber, 0);
  // 短窗口（<=15分钟）固定 6 个时间刻度点：
  // 例如 15:50-16:00 => 15:50 15:52 15:54 15:56 15:58 16:00
  if (range > 0 && range <= 15 * 60 * 1000) {
    const minute = 60_000;
    // 6个点 => 5段；将刻度间隔对齐到整分钟，避免出现“全不显示/错位”的情况
    const stepMs = Math.max(Math.round((range / 5) / minute) * minute, minute);
    return {
      min: Math.floor(minNumber / minute) * minute,
      max: maxNumber,
      splitNumber: 5,
      interval: stepMs,
      minInterval: stepMs,
      maxInterval: stepMs
    };
  }

  return { min, max };
}

const render = async () => {
  if (!chartRef.value || !props.dataSeries) return;
  if (!echarts) await ensureEcharts();
  if (!chartInstance.value) {
    chartInstance.value = echarts!.init(chartRef.value);
  }
  const colors = getThemeColors();
  const deviceMap: Record<string, [number, number][]> = {};
  props.dataSeries.forEach(point => {
    const raw = point[props.metric];
    if (raw == null) return;
    const value = typeof raw === 'number' ? raw : Number(raw);
    if (!Number.isFinite(value)) return;
    if (!deviceMap[point.deviceId]) deviceMap[point.deviceId] = [];
    deviceMap[point.deviceId].push([point.timestamp, value]);
  });

  const thresholdLines = (() => {
    if (!props.showThresholds) return [];
    const season = ruleStore.currentSeason?.season;
    if (!season) return [];

    const metric = props.metric as unknown as string;
    const relevant = ruleStore.systemTemplates.filter(t => t.season === season && t.metric === metric && t.enabled);
    return relevant.map(t => ({
      name: `${t.name}（${t.threshold}）`,
      yAxis: t.threshold,
      lineStyle: {
        type: 'dashed',
        color: getLevelColor(t.level),
        width: 2
      },
      label: {
        formatter: `{b}`,
        position: 'insideEndTop'
      }
    }));
  })();

  const metricName: Record<string, string> = {
    temperature: '温度',
    humidity: '湿度',
    pressure: '气压',
    light: '光照'
  };

  const timeAxisTicks: any = getTimeAxisTicks();

  const baseSeries = Object.entries(deviceMap).map(([deviceId, data], index) => {
    const sorted = data.sort((a, b) => a[0] - b[0]);
    const withGaps = insertGapBreaks(sorted);
    const s: any = {
      name: deviceId,
      type: 'line',
      showSymbol: false,
      smooth: true,
      connectNulls: false,
      data: sampleData(withGaps)
    };
    if (index === 0 && thresholdLines.length) {
      s.markLine = {
        silent: true,
        symbol: 'none',
        data: thresholdLines
      };
    }
    return s;
  });

  const series = baseSeries.length
    ? baseSeries
    : thresholdLines.length
      ? [
          {
            name: '阈值',
            type: 'line',
            data: [],
            showSymbol: false,
            lineStyle: { opacity: 0 },
            markLine: { silent: true, symbol: 'none', data: thresholdLines }
          }
        ]
      : [];

  chartInstance.value.setOption(
    {
      tooltip: {
        trigger: 'axis',
        axisPointer: { type: 'cross' },
        backgroundColor: colors.surface,
        borderColor: colors.borderPrimary,
        textStyle: { color: colors.textPrimary }
      },
      legend: {
        bottom: 10,
        icon: 'circle',
        itemWidth: 10,
        itemHeight: 10,
        textStyle: { fontSize: window.innerWidth <= 768 ? 11 : 12, color: colors.textSecondary },
        type: window.innerWidth <= 480 ? 'scroll' : 'plain'
      },
      grid: {
        top: 36,
        left: window.innerWidth <= 480 ? 30 : 40,
        right: window.innerWidth <= 480 ? 10 : 20,
        bottom: 70
      },
      xAxis: {
        type: 'time',
        ...timeAxisTicks,
        axisLine: { lineStyle: { color: colors.borderPrimary } },
        axisLabel: {
          color: colors.textSecondary,
          formatter: timeAxisLabelFormatter,
          hideOverlap: true,
          showMinLabel: true,
          showMaxLabel: true
        },
        splitLine: { lineStyle: { color: colors.borderPrimary, opacity: 0.4 } }
      },
      yAxis: {
        type: 'value',
        name: metricName[props.metric] || props.metric,
        nameTextStyle: { fontSize: 12, color: colors.textSecondary },
        axisLine: { lineStyle: { color: colors.borderPrimary } },
        axisLabel: { color: colors.textSecondary },
        splitLine: { lineStyle: { color: colors.borderPrimary, opacity: 0.35 } }
      },
      dataZoom: [
        {
          type: 'inside',
          start: 0,
          end: 100
        }
      ],
      series
    },
    true
  );
};

const updateAxisRange = () => {
  if (!chartInstance.value) return;
  chartInstance.value.setOption(
    {
      xAxis: getTimeAxisTicks()
    },
    false
  );
};

function getLevelColor(level: string): string {
  const colors: Record<string, string> = {
    critical: '#f56c6c',
    warning: '#e6a23c',
    info: '#909399'
  };
  return colors[level] || '#909399';
}

// 使用防抖优化render和resize
const debouncedRender = useDebounceFn(render, 200);
const debouncedUpdateAxisRange = useDebounceFn(updateAxisRange, 200);
const debouncedResize = useDebounceFn(() => {
  chartInstance.value?.resize();
}, 100);

onMounted(async () => {
  if (props.showThresholds) {
    try {
      await Promise.all([
        ruleStore.currentSeason ? Promise.resolve(ruleStore.currentSeason) : ruleStore.fetchCurrentSeason(),
        ruleStore.systemTemplates.length ? Promise.resolve(ruleStore.systemTemplates) : ruleStore.fetchSystemTemplates()
      ]);
    } catch (e) {
      console.warn('Failed to load seasonal thresholds:', e);
    }
  }
  await render();
  window.addEventListener('resize', debouncedResize);
});

onUnmounted(() => {
  window.removeEventListener('resize', debouncedResize);
  chartInstance.value?.dispose();
  chartInstance.value = null;
});

// 注意：dataSeries 通常是同一个数组引用（push追加），仅 watch 引用不会触发；需要监听 length 才能实时重绘
watch(
  () => [props.dataSeries.length, props.dataSeries[props.dataSeries.length - 1]?.timestamp],
  () => debouncedRender()
);

watch(
  () => props.dataSeries,
  () => debouncedRender()
);

watch(
  () => [props.xAxisMin, props.xAxisMax],
  () => debouncedUpdateAxisRange()
);

// 当主题切换时（light/dark/auto），重新渲染以更新图例/坐标轴颜色
watch(
  () => appStore.theme,
  () => debouncedRender()
);

watch(
  () => [ruleStore.currentSeason, ruleStore.systemTemplates],
  () => {
    if (props.showThresholds) debouncedRender();
  },
  { deep: true }
);
</script>

<style scoped>
.chart {
  width: 100%;
  height: 320px;
}
.chart-title {
  font-weight: 700;
  margin: 6px 0 12px;
}

/* 移动端优化 */
@media (max-width: 768px) {
  .chart {
    height: 260px;
  }

  .chart-title {
    font-size: 15px;
    margin: 4px 0 10px;
  }
}

@media (max-width: 480px) {
  .chart {
    height: 220px;
  }

  .chart-title {
    font-size: 14px;
  }
}
</style>
