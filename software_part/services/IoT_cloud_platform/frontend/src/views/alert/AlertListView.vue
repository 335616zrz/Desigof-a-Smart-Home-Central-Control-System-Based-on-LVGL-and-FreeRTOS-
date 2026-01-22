<template>
  <div class="page-shell page-padding">
    <el-alert
      v-if="ruleStore.currentSeason"
      type="info"
      :closable="false"
      show-icon
      style="margin-bottom: 12px;"
    >
      <template #title>当前季节：{{ ruleStore.currentSeason.seasonName }}</template>
      <span>系统模板会按当前季节参与告警评估，可在下方启用/禁用模板。</span>
    </el-alert>

    <section class="app-hero">
      <div class="app-hero__content">
        <p class="eyebrow">告警中心</p>
        <h1>实时告警 · 规则治理</h1>
        <p class="lede">查看告警、确认处理并管理触发规则。</p>

        <div class="app-hero__badges">
          <el-tag effect="dark" type="warning">开放告警 {{ openAlerts }}</el-tag>
          <el-tag effect="plain" type="danger">严重 {{ criticalAlerts }}</el-tag>
          <el-tag effect="plain" type="info">规则 {{ ruleCount }}</el-tag>
          <el-tag v-if="ruleStore.currentSeason" effect="plain" :type="seasonTagType(ruleStore.currentSeason.season)">
            {{ ruleStore.currentSeason.seasonName }}模式
          </el-tag>
        </div>

        <div class="app-hero__actions">
          <el-button type="primary" @click="loadAlerts">刷新告警</el-button>
          <el-button plain @click="openCreateRule">新建规则</el-button>
          <el-button text @click="loadRules">刷新规则</el-button>
        </div>
      </div>

      <div class="app-hero__panel">
        <div class="app-mini-grid">
          <div class="app-mini-card">
            <div class="app-chip-icon danger"><el-icon><WarningFilled /></el-icon></div>
            <div class="mini-meta">
              <p>严重告警</p>
              <strong>{{ criticalAlerts }}</strong>
              <span>优先处理</span>
            </div>
          </div>
          <div class="app-mini-card">
            <div class="app-chip-icon warning"><el-icon><Bell /></el-icon></div>
            <div class="mini-meta">
              <p>开放告警</p>
              <strong>{{ openAlerts }}</strong>
              <span>处理中/未关闭</span>
            </div>
          </div>
          <div class="app-mini-card">
            <div class="app-chip-icon info"><el-icon><List /></el-icon></div>
            <div class="mini-meta">
              <p>规则数量</p>
              <strong>{{ ruleCount }}</strong>
              <span>启用/停用可切换</span>
            </div>
          </div>
        </div>
      </div>
    </section>

    <el-card shadow="never">
      <el-tabs v-model="activeTab" @tab-change="syncTabToRoute">
        <el-tab-pane label="告警列表" name="alerts">
          <div
            class="app-section-card"
            style="margin-bottom: 12px; display: flex; justify-content: space-between; align-items: flex-end; flex-wrap: wrap; gap: 10px;"
          >
            <el-form :inline="true" class="filter" :model="filters" @submit.prevent>
              <el-form-item label="级别">
                <el-select v-model="filters.level" placeholder="全部" clearable style="width: 140px">
                  <el-option label="严重" value="critical" />
                  <el-option label="警告" value="warning" />
                  <el-option label="提示" value="info" />
                </el-select>
              </el-form-item>
              <el-form-item label="状态">
                <el-select v-model="filters.status" placeholder="全部" clearable style="width: 140px">
                  <el-option label="未处理" value="open" />
                  <el-option label="已确认" value="ack" />
                  <el-option label="已关闭" value="closed" />
                </el-select>
              </el-form-item>
              <el-form-item>
                <el-button type="primary" @click="loadAlerts">筛选</el-button>
              </el-form-item>
            </el-form>

            <el-button type="primary" :loading="exporting" @click="handleExport">导出Excel</el-button>
          </div>

          <el-table :data="alertStore.items" :loading="alertStore.loading" border stripe>
            <el-table-column prop="deviceId" label="设备" width="140" />
            <el-table-column prop="level" label="级别" width="100">
              <template #default="{ row }">
                <el-tag :type="levelType(row.level)" effect="dark" size="small">{{ levelLabel(row.level) }}</el-tag>
              </template>
            </el-table-column>
            <el-table-column prop="message" label="信息" min-width="150" />
            <el-table-column prop="status" label="状态" width="100" class-name="mobile-hide">
              <template #default="{ row }">
                <el-tag :type="statusType(row.status)" size="small">{{ statusLabel(row.status) }}</el-tag>
              </template>
            </el-table-column>
            <el-table-column prop="createdAt" label="创建时间" width="160" :formatter="fmt" class-name="mobile-hide" />
            <el-table-column label="操作" width="200" fixed="right">
              <template #default="{ row }">
                <el-button link type="primary" size="small" @click="viewDetail(row.id)">详情</el-button>
                <el-button link type="warning" size="small" @click="ack(row.id)" :disabled="row.status !== 'open'">确认</el-button>
                <el-button link type="success" size="small" @click="close(row.id)" :disabled="row.status === 'closed'">关闭</el-button>
              </template>
            </el-table-column>
          </el-table>
        </el-tab-pane>

        <el-tab-pane label="告警规则" name="rules">
          <div class="flex-between" style="margin-bottom: 12px; gap: 10px; flex-wrap: wrap;">
            <div style="display:flex; gap:8px; align-items:center; flex-wrap: wrap;">
              <el-button type="primary" @click="openCreateRule">新建自定义规则</el-button>
              <el-button @click="loadRules">刷新规则</el-button>
            </div>
          </div>

          <el-collapse v-model="activeCollapse">
            <el-collapse-item name="templates" title="系统预置模板（基于国家标准）">
              <el-alert type="info" :closable="false" show-icon style="margin-bottom: 12px;">
                <template #title>系统模板仅支持启用/禁用</template>
                <span>阈值与说明由系统维护，不支持删除或修改。</span>
              </el-alert>

              <div style="margin-bottom: 12px;">
                <el-radio-group v-model="seasonFilter">
                  <el-radio-button label="">全部季节</el-radio-button>
                  <el-radio-button label="summer">夏季</el-radio-button>
                  <el-radio-button label="winter">冬季</el-radio-button>
                  <el-radio-button label="transition">过渡季</el-radio-button>
                </el-radio-group>
              </div>

              <el-table :data="filteredTemplates" :loading="ruleStore.loading" border stripe>
                <el-table-column prop="season" label="季节" width="100">
                  <template #default="{ row }">
                    <el-tag size="small" effect="dark" :type="seasonTagType(row.season)">
                      {{ seasonLabel(row.season) }}
                    </el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="name" label="名称" min-width="160" />
                <el-table-column prop="metric" label="指标" width="110">
                  <template #default="{ row }">
                    <el-tag size="small" effect="plain" :type="row.metric === 'temperature' ? 'warning' : 'info'">
                      {{ metricLabel(row.metric) }}
                    </el-tag>
                  </template>
                </el-table-column>
                <el-table-column label="条件" min-width="190">
                  <template #default="{ row }">
                    {{ row.operator }} {{ row.threshold }}{{ metricUnit(row.metric) }}
                    <el-tag
                      v-if="row.conditionType"
                      size="small"
                      effect="plain"
                      :type="row.conditionType === 'above' ? 'danger' : 'info'"
                      style="margin-left: 6px;"
                    >
                      {{ row.conditionType === 'above' ? '过高' : '过低' }}
                    </el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="level" label="级别" width="100">
                  <template #default="{ row }">
                    <el-tag :type="levelType(row.level)" effect="dark" size="small">{{ levelLabel(row.level) }}</el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="durationSeconds" label="持续" width="90">
                  <template #default="{ row }">
                    {{ row.durationSeconds || 0 }}s
                  </template>
                </el-table-column>
                <el-table-column prop="enabled" label="启用" width="80">
                  <template #default="{ row }">
                    <el-switch v-model="row.enabled" @change="val => onToggleTemplate(row, val)" size="small" />
                  </template>
                </el-table-column>
                <el-table-column prop="description" label="说明" min-width="200" class-name="mobile-hide" />
              </el-table>
            </el-collapse-item>

            <el-collapse-item name="userRules" title="用户自定义规则">
              <el-table :data="ruleStore.userRules" :loading="ruleStore.loading" border stripe>
                <el-table-column prop="name" label="名称" min-width="140" />
                <el-table-column prop="deviceId" label="设备" min-width="110" class-name="mobile-hide">
                  <template #default="{ row }">
                    <el-tag v-if="row.deviceId" type="info" size="small">{{ row.deviceId }}</el-tag>
                    <el-tag v-else type="success" effect="plain" size="small">全部</el-tag>
                  </template>
                </el-table-column>
                <el-table-column label="条件" min-width="170">
                  <template #default="{ row }">
                    {{ metricLabel(row.metric) }} {{ row.operator }} {{ row.threshold }}{{ metricUnit(row.metric) }}
                    <span v-if="row.durationSeconds"> / {{ row.durationSeconds }}s</span>
                  </template>
                </el-table-column>
                <el-table-column prop="level" label="级别" width="100">
                  <template #default="{ row }">
                    <el-tag :type="levelType(row.level)" effect="dark" size="small">{{ levelLabel(row.level) }}</el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="enabled" label="启用" width="70">
                  <template #default="{ row }">
                    <el-switch v-model="row.enabled" @change="val => onToggleUserRule(row, val)" size="small" />
                  </template>
                </el-table-column>
                <el-table-column label="操作" width="160" fixed="right">
                  <template #default="{ row }">
                    <el-button link type="primary" size="small" @click="openEditRule(row)">编辑</el-button>
                    <el-popconfirm title="确定删除该规则？" @confirm="removeRule(row.id!)">
                      <template #reference>
                        <el-button link type="danger" size="small">删除</el-button>
                      </template>
                    </el-popconfirm>
                  </template>
                </el-table-column>
              </el-table>
            </el-collapse-item>
          </el-collapse>
        </el-tab-pane>
      </el-tabs>
    </el-card>

    <el-dialog v-model="dialogVisible" :title="dialogTitle" width="520px" destroy-on-close>
      <AlertRuleForm
        ref="formRef"
        :model-value="form"
        @update:model-value="val => Object.assign(form, val)"
      />
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="saving" @click="saveRule">保存</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, reactive, ref } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { Bell, List, WarningFilled } from '@element-plus/icons-vue';
import { ElMessage } from 'element-plus';
import AlertRuleForm from '@/components/AlertRuleForm.vue';
import { exportAlerts } from '@/api/alert';
import { useAlertRuleStore } from '@/stores/alertRule';
import { useAlertStore } from '@/stores/alert';
import { formatDateTime } from '@/utils/format';
import type { AlertRule } from '@/types/models';

const alertStore = useAlertStore();
const ruleStore = useAlertRuleStore();
const route = useRoute();
const router = useRouter();

const activeTab = ref((route.query.tab as string) || 'alerts');
const filters = reactive<{ level?: string; status?: string }>({});
const exporting = ref(false);
const dialogVisible = ref(false);
const saving = ref(false);
const editingId = ref<number | null>(null);
const formRef = ref<InstanceType<typeof AlertRuleForm>>();
const activeCollapse = ref<string[]>(['templates', 'userRules']);
const seasonFilter = ref<string>('');

const form = reactive<AlertRule>({
  name: '',
  description: '',
  deviceId: '',
  metric: 'temperature',
  operator: '>',
  threshold: 30,
  durationSeconds: 0,
  level: 'warning',
  enabled: true
});

const dialogTitle = computed(() => (editingId.value ? '编辑规则' : '新建规则'));

const openAlerts = computed(() => alertStore.items.filter(a => a.status === 'open').length);
const criticalAlerts = computed(() => alertStore.items.filter(a => a.level === 'critical' && a.status === 'open').length);
const ruleCount = computed(() => ruleStore.systemTemplates.length + ruleStore.userRules.length);

const filteredTemplates = computed(() => {
  if (!seasonFilter.value) return ruleStore.systemTemplates;
  return ruleStore.systemTemplates.filter(t => t.season === seasonFilter.value);
});

const levelType = (level: string) => (level === 'critical' ? 'danger' : level === 'warning' ? 'warning' : 'info');
const levelLabel = (level: string) => (level === 'critical' ? '严重' : level === 'warning' ? '警告' : '提示');
const statusType = (status: string) => (status === 'open' ? 'danger' : status === 'ack' ? 'warning' : 'success');
const statusLabel = (status: string) => (status === 'open' ? '未处理' : status === 'ack' ? '已确认' : '已关闭');
const fmt = (_: unknown, __: unknown, value: string | number) => formatDateTime(value);

function showApiError(error: any, fallback: string) {
  const status = error?.response?.status;
  const data = error?.response?.data;
  const message =
    (typeof data === 'string' ? data : data?.message) ||
    error?.message ||
    fallback;

  if (status === 401) {
    ElMessage.error('登录已过期，请重新登录');
    return;
  }

  if (status === 403) {
    ElMessage.error('权限不足：当前账号无法执行该操作');
    return;
  }

  ElMessage.error(message);
}

async function loadAlerts() {
  try {
    await alertStore.fetchList({ pageSize: 100, level: filters.level, status: filters.status });
  } catch (error: any) {
    showApiError(error, '加载告警失败');
  }
}

async function loadRules() {
  try {
    await Promise.all([
      ruleStore.fetchCurrentSeason(),
      ruleStore.fetchSystemTemplates(),
      ruleStore.fetchUserRules()
    ]);
  } catch (error: any) {
    showApiError(error, '加载规则失败');
  }
}

async function handleExport() {
  exporting.value = true;
  try {
    await exportAlerts({
      status: filters.status || undefined,
      level: filters.level || undefined
    });
    ElMessage.success('告警记录已导出');
  } catch (error: any) {
    showApiError(error, '导出失败');
  } finally {
    exporting.value = false;
  }
}

async function ack(id: number) {
  try {
    await alertStore.acknowledge(id);
    await loadAlerts();
  } catch (error: any) {
    showApiError(error, '确认告警失败');
  }
}

async function close(id: number) {
  try {
    await alertStore.close(id);
    await loadAlerts();
  } catch (error: any) {
    showApiError(error, '关闭告警失败');
  }
}

function viewDetail(id: number) {
  router.push({ name: 'alert-detail', params: { id } });
}

function resetForm() {
  Object.assign(form, {
    name: '',
    description: '',
    deviceId: '',
    metric: 'temperature',
    operator: '>',
    threshold: 30,
    durationSeconds: 0,
    level: 'warning',
    enabled: true
  });
}

function openCreateRule() {
  editingId.value = null;
  resetForm();
  dialogVisible.value = true;
}

function openEditRule(rule: AlertRule) {
  editingId.value = rule.id ?? null;
  Object.assign(form, rule);
  dialogVisible.value = true;
}

async function saveRule() {
  const valid = await formRef.value?.validate();
  if (!valid) return;
  saving.value = true;
  try {
    if (editingId.value) {
      await ruleStore.update(editingId.value, { ...form });
      ElMessage.success('规则已更新');
    } else {
      await ruleStore.create({ ...form });
      ElMessage.success('规则已创建');
    }
    dialogVisible.value = false;
    await ruleStore.fetchUserRules();
  } catch (error: any) {
    showApiError(error, '保存规则失败');
  } finally {
    saving.value = false;
  }
}

async function onToggleTemplate(rule: AlertRule, enabled: any) {
  if (!rule.id) return;
  try {
    await ruleStore.toggle(rule.id, Boolean(enabled));
    ElMessage.success(Boolean(enabled) ? '已启用模板' : '已停用模板');
  } catch (error: any) {
    showApiError(error, '切换规则状态失败');
    await ruleStore.fetchSystemTemplates();
  }
}

async function onToggleUserRule(rule: AlertRule, enabled: any) {
  if (!rule.id) return;
  try {
    await ruleStore.toggle(rule.id, Boolean(enabled));
    ElMessage.success(Boolean(enabled) ? '已启用' : '已停用');
  } catch (error: any) {
    showApiError(error, '切换规则状态失败');
    await ruleStore.fetchUserRules();
  }
}

async function removeRule(id: number) {
  try {
    await ruleStore.remove(id);
    ElMessage.success('已删除');
    await ruleStore.fetchUserRules();
  } catch (error: any) {
    showApiError(error, '删除规则失败');
  }
}

function syncTabToRoute(name: string) {
  router.replace({ query: { ...route.query, tab: name } });
}

onMounted(async () => {
  await Promise.all([loadAlerts(), loadRules()]);
});

function seasonLabel(season: string | null | undefined) {
  return season === 'summer' ? '夏季' : season === 'winter' ? '冬季' : season === 'transition' ? '过渡季' : '全年';
}

function seasonTagType(season: string | null | undefined): 'danger' | 'warning' | 'info' {
  if (season === 'summer') return 'danger';
  if (season === 'transition') return 'warning';
  return 'info';
}

function metricLabel(metric: string) {
  const labels: Record<string, string> = {
    temperature: '温度',
    humidity: '湿度',
    pressure: '气压',
    light: '光照'
  };
  return labels[metric] || metric;
}

function metricUnit(metric: string) {
  const units: Record<string, string> = {
    temperature: '°C',
    humidity: '%',
    pressure: 'hPa',
    light: 'lux'
  };
  return units[metric] || '';
}
</script>

<style scoped>
.filter {
  background: transparent;
  padding: 4px 0;
}

@media (max-width: 768px) {
  .filter.el-form--inline .el-form-item {
    display: block;
    margin-bottom: 10px;
  }

  .filter .el-select,
  .filter .el-button {
    width: 100%;
  }

  .app-hero__actions {
    display: flex;
    flex-direction: column;
    gap: 10px;
  }

  .app-hero__actions .el-button {
    width: 100%;
  }

  .flex-between {
    flex-direction: column;
    align-items: stretch;
  }

  .flex-between > div {
    width: 100%;
    margin-bottom: 8px;
  }
}
</style>
