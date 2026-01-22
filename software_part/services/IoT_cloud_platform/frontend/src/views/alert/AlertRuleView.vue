<template>
  <div class="page-padding">
      <div class="flex-between" style="margin-bottom: 12px; gap: 10px; flex-wrap: wrap;">
        <div class="flex-center" style="gap: 10px;">
          <h2 class="section-title">告警规则</h2>
        <el-tag>{{ userRules.length }} 条</el-tag>
      </div>
      <div style="display: flex; gap: 8px;">
        <el-select v-model="filterEnabled" placeholder="全部状态" clearable style="width: 160px" @change="load">
          <el-option label="仅启用" :value="true" />
          <el-option label="仅停用" :value="false" />
        </el-select>
        <el-button type="primary" @click="openCreate">新建规则</el-button>
        <el-button @click="load">刷新</el-button>
      </div>
    </div>

    <el-table :data="userRules" :loading="ruleStore.loading" border>
      <el-table-column prop="name" label="名称" min-width="160" />
      <el-table-column prop="deviceId" label="设备" min-width="120">
        <template #default="{ row }">
          <el-tag v-if="row.deviceId" type="info">{{ row.deviceId }}</el-tag>
          <el-tag v-else type="success" effect="plain">全部</el-tag>
        </template>
      </el-table-column>
      <el-table-column prop="metric" label="指标" width="110" />
      <el-table-column label="条件" min-width="140">
        <template #default="{ row }">
          {{ row.metric }} {{ row.operator }} {{ row.threshold }}
          <span v-if="row.durationSeconds"> / {{ row.durationSeconds }}s</span>
        </template>
      </el-table-column>
      <el-table-column prop="level" label="级别" width="110">
        <template #default="{ row }">
          <el-tag :type="levelType(row.level)" effect="dark">{{ levelLabel(row.level) }}</el-tag>
        </template>
      </el-table-column>
      <el-table-column prop="enabled" label="启用" width="90">
        <template #default="{ row }">
          <el-switch :model-value="row.enabled" @change="val => onToggle(row, Boolean(val))" />
        </template>
      </el-table-column>
      <el-table-column label="操作" width="180">
        <template #default="{ row }">
          <el-button link type="primary" @click="openEdit(row)">编辑</el-button>
          <el-popconfirm title="确定删除该规则？" @confirm="remove(row.id!)">
            <template #reference>
              <el-button link type="danger">删除</el-button>
            </template>
          </el-popconfirm>
        </template>
      </el-table-column>
    </el-table>

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
import { ElMessage } from 'element-plus';
import AlertRuleForm from '@/components/AlertRuleForm.vue';
import { useAlertRuleStore } from '@/stores/alertRule';
import type { AlertRule } from '@/types/models';

const ruleStore = useAlertRuleStore();
const filterEnabled = ref<boolean | undefined>(undefined);
const dialogVisible = ref(false);
const saving = ref(false);
const editingId = ref<number | null>(null);
const formRef = ref<InstanceType<typeof AlertRuleForm>>();

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

const userRules = computed(() => ruleStore.rules.filter(r => !r.isSystemTemplate));

const levelType = (level: string) => (level === 'critical' ? 'danger' : level === 'warning' ? 'warning' : 'info');
const levelLabel = (level: string) => (level === 'critical' ? '严重' : level === 'warning' ? '警告' : '提示');

async function load() {
  await ruleStore.fetchRules(filterEnabled.value);
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

function openCreate() {
  editingId.value = null;
  resetForm();
  dialogVisible.value = true;
}

function openEdit(rule: AlertRule) {
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
    await load();
  } catch (error: any) {
    ElMessage.error(error.message || '操作失败');
  } finally {
    saving.value = false;
  }
}

async function onToggle(rule: AlertRule, nextEnabled: boolean) {
  if (!rule.id) return;
  const originalState = rule.enabled;
  rule.enabled = nextEnabled;

  try {
    await ruleStore.toggle(rule.id, nextEnabled);
    ElMessage.success(nextEnabled ? '已启用' : '已停用');
  } catch (error: any) {
    rule.enabled = originalState;
    ElMessage.error(error.message || '操作失败');
  }
}

async function remove(id: number) {
  try {
    await ruleStore.remove(id);
    ElMessage.success('已删除');
    await load();
  } catch (error: any) {
    ElMessage.error(error.message || '操作失败');
  }
}

onMounted(load);
</script>

<style scoped>
.el-table {
  margin-top: 10px;
}
</style>
