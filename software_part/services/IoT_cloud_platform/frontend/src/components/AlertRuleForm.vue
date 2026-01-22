<template>
  <el-form ref="formRef" :model="localRule" :rules="rules" label-width="96px" status-icon>
    <el-form-item label="规则名称" prop="name">
      <el-input v-model="localRule.name" placeholder="如：温度过高" />
    </el-form-item>
    <el-form-item label="作用设备" prop="deviceId">
      <el-input v-model="localRule.deviceId" placeholder="留空表示所有设备" clearable />
    </el-form-item>
    <el-form-item label="监控指标" prop="metric">
      <el-select v-model="localRule.metric" placeholder="选择指标" style="width: 180px">
        <el-option label="温度" value="temperature" />
        <el-option label="湿度" value="humidity" />
        <el-option label="气压" value="pressure" />
        <el-option label="光照" value="light" />
      </el-select>
    </el-form-item>
    <el-form-item label="阈值条件" required>
      <div class="condition">
        <el-select v-model="localRule.operator" style="width: 120px">
          <el-option label=">" value=">" />
          <el-option label="<" value="<" />
          <el-option label=">=" value=">=" />
          <el-option label="<=" value="<=" />
          <el-option label="==" value="==" />
          <el-option label="!=" value="!=" />
        </el-select>
        <el-input-number v-model="localRule.threshold" :step="0.1" style="width: 160px" />
        <span class="unit">阈值</span>
      </div>
    </el-form-item>
    <el-form-item label="持续时间">
      <el-input-number v-model="localRule.durationSeconds" :min="0" :step="30" style="width: 180px" />
      <span class="hint">秒，0 表示立即触发</span>
    </el-form-item>
    <el-form-item label="告警级别" prop="level">
      <el-radio-group v-model="localRule.level">
        <el-radio-button label="critical">严重</el-radio-button>
        <el-radio-button label="warning">警告</el-radio-button>
        <el-radio-button label="info">提示</el-radio-button>
      </el-radio-group>
    </el-form-item>
    <el-form-item label="描述">
      <el-input v-model="localRule.description" type="textarea" rows="2" placeholder="可选备注" />
    </el-form-item>
    <el-form-item label="启用">
      <el-switch v-model="localRule.enabled" />
    </el-form-item>
  </el-form>
</template>

<script setup lang="ts">
import { reactive, ref, watch } from 'vue';
import type { FormInstance, FormRules } from 'element-plus';
import type { AlertRule } from '@/types/models';

const props = defineProps<{
  modelValue: AlertRule;
}>();

const emit = defineEmits<{
  (e: 'update:modelValue', value: AlertRule): void;
}>();

const formRef = ref<FormInstance>();
const localRule = reactive<AlertRule>({ ...props.modelValue });

watch(
  () => props.modelValue,
  val => Object.assign(localRule, val),
  { deep: true }
);

watch(
  () => localRule,
  val => emit('update:modelValue', { ...val }),
  { deep: true }
);

const rules: FormRules<AlertRule> = {
  name: [{ required: true, message: '请输入规则名称', trigger: 'blur' }],
  metric: [{ required: true, message: '请选择指标', trigger: 'change' }],
  operator: [{ required: true, message: '请选择比较符', trigger: 'change' }],
  threshold: [{ required: true, message: '请输入阈值', trigger: 'change', type: 'number' }],
  level: [{ required: true, message: '请选择告警级别', trigger: 'change' }]
};

defineExpose({
  validate: () => formRef.value?.validate()
});
</script>

<style scoped>
.condition {
  display: flex;
  align-items: center;
  gap: 8px;
}
.unit {
  color: var(--text-secondary);
}
.hint {
  margin-left: 10px;
  color: var(--text-secondary);
}
</style>
