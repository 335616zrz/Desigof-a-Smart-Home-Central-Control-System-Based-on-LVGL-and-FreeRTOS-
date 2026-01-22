<template>
  <div class="page-padding" v-loading="alertStore.currentLoading">
    <div class="flex-between" style="margin-bottom: 12px; gap: 10px; flex-wrap: wrap;">
      <div class="flex-center" style="gap: 10px;">
        <h2 class="section-title">告警详情</h2>
        <AlertBadge v-if="alert" :level="alert.level" :status="alert.status" />
      </div>
      <div>
        <el-button type="primary" @click="acknowledge" :disabled="!alert || alert.status !== 'open'">确认</el-button>
        <el-button type="success" @click="close" :disabled="!alert || alert.status === 'closed'">关闭</el-button>
      </div>
    </div>

    <el-row :gutter="14">
      <el-col :xs="24" :md="14">
        <el-card shadow="never">
          <template #header>
            <div class="flex-between">
              <span>基本信息</span>
              <el-link v-if="alert" type="primary" :underline="false" @click="gotoDevice(alert.deviceId)">查看设备</el-link>
            </div>
          </template>
          <el-descriptions :column="2" border v-if="alert">
            <el-descriptions-item label="ID">{{ alert.id }}</el-descriptions-item>
            <el-descriptions-item label="设备">{{ alert.deviceId }}</el-descriptions-item>
            <el-descriptions-item label="消息" :span="2">{{ alert.message }}</el-descriptions-item>
            <el-descriptions-item label="级别">{{ levelLabel(alert.level) }}</el-descriptions-item>
            <el-descriptions-item label="状态">{{ statusLabel(alert.status) }}</el-descriptions-item>
            <el-descriptions-item label="创建时间">{{ fmt(alert.createdAt) }}</el-descriptions-item>
            <el-descriptions-item label="确认时间">{{ alert.acknowledgedAt ? fmt(alert.acknowledgedAt) : '-' }}</el-descriptions-item>
            <el-descriptions-item label="关闭时间">{{ alert.closedAt ? fmt(alert.closedAt) : '-' }}</el-descriptions-item>
          </el-descriptions>
          <el-empty v-else description="正在加载告警..." />
        </el-card>

        <el-card shadow="never" style="margin-top: 12px;">
          <template #header><span>时间线</span></template>
          <el-timeline>
            <el-timeline-item v-if="alert" :timestamp="fmt(alert.createdAt)" type="danger" color="#ef4444">
              创建：{{ alert.message }}
            </el-timeline-item>
            <el-timeline-item
              v-if="alert?.acknowledgedAt"
              :timestamp="fmt(alert.acknowledgedAt)"
              type="warning"
              color="#f59e0b"
            >
              已确认
            </el-timeline-item>
            <el-timeline-item
              v-if="alert?.closedAt"
              :timestamp="fmt(alert.closedAt)"
              type="success"
              color="#10b981"
            >
              已关闭
            </el-timeline-item>
          </el-timeline>
        </el-card>
      </el-col>

      <el-col :xs="24" :md="10">
        <el-card shadow="never">
          <template #header><span>关联规则</span></template>
          <div v-if="rule">
            <p class="rule-title">{{ rule.name }}</p>
            <p class="muted">{{ rule.description || '无描述' }}</p>
            <div class="rule-meta">
              <el-tag size="small" type="info">{{ rule.metric }} {{ rule.operator }} {{ rule.threshold }}</el-tag>
              <el-tag size="small" type="warning" v-if="rule.durationSeconds && rule.durationSeconds > 0">
                持续 {{ rule.durationSeconds }}s
              </el-tag>
              <el-tag size="small" :type="levelType(rule.level)">{{ levelLabel(rule.level) }}</el-tag>
              <el-tag size="small" :type="rule.enabled ? 'success' : 'info'">{{ rule.enabled ? '启用' : '停用' }}</el-tag>
            </div>
          </div>
          <el-empty v-else description="无关联规则或未找到" />
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { ElMessage } from 'element-plus';
import AlertBadge from '@/components/AlertBadge.vue';
import { useAlertStore } from '@/stores/alert';
import { useAlertRuleStore } from '@/stores/alertRule';
import { formatDateTime } from '@/utils/format';

const route = useRoute();
const router = useRouter();
const alertStore = useAlertStore();
const ruleStore = useAlertRuleStore();

const alert = computed(() => alertStore.current);
const rule = computed(() => ruleStore.current);

const fmt = (val: string | number | undefined) => (val ? formatDateTime(val) : '-');
const levelLabel = (level: string) => (level === 'critical' ? '严重' : level === 'warning' ? '警告' : '提示');
const statusLabel = (status: string) => (status === 'open' ? '未处理' : status === 'ack' ? '已确认' : '已关闭');
const levelType = (level: string) => (level === 'critical' ? 'danger' : level === 'warning' ? 'warning' : 'info');

async function load() {
  const id = Number(route.params.id);
  if (!id) return;
  const data = await alertStore.fetchOne(id);
  if (data?.ruleId) {
    await ruleStore.fetchOne(data.ruleId);
  }
}

async function acknowledge() {
  if (!alert.value) return;
  await alertStore.acknowledge(alert.value.id);
  ElMessage.success('已确认');
}

async function close() {
  if (!alert.value) return;
  await alertStore.close(alert.value.id);
  ElMessage.success('已关闭');
}

function gotoDevice(deviceId: string) {
  router.push({ name: 'device-detail', params: { id: deviceId } });
}

onMounted(load);
</script>

<style scoped>
.rule-title {
  font-weight: 600;
  margin: 0 0 6px;
}
.rule-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
  margin-top: 8px;
}
</style>
