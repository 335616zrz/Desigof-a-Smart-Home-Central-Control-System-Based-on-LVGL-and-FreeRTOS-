<template>
  <el-dialog v-model="visible" :title="device ? '编辑设备' : '新建设备'" width="520px">
    <el-form :model="form" label-width="110px" :rules="rules" ref="formRef">
      <el-form-item label="设备ID" prop="deviceId">
        <el-input v-model="form.deviceId" :disabled="Boolean(device)" />
      </el-form-item>
      <el-form-item label="名称" prop="name">
        <el-input v-model="form.name" />
      </el-form-item>
      <el-form-item label="固件版本">
        <el-input v-model="form.firmwareVersion" />
      </el-form-item>
    </el-form>
    <template #footer>
      <el-button @click="visible = false">取消</el-button>
      <el-button
        type="primary"
        :loading="saving"
        :disabled="saveSuccess"
        :class="{ 'anim-pulse': saveSuccess }"
        @click="submit"
      >
        {{ saveSuccess ? '保存成功' : '保存' }}
      </el-button>
    </template>
  </el-dialog>
</template>

<script setup lang="ts">
import { computed, reactive, ref, watch } from 'vue';
import { useDeviceStore } from '@/stores/device';

const props = defineProps<{ modelValue: boolean; device?: any }>();
const emit = defineEmits(['update:modelValue', 'submitted']);

const store = useDeviceStore();
const formRef = ref();
const saving = ref(false);
const saveSuccess = ref(false);
const form = reactive({ deviceId: '', name: '', firmwareVersion: '' });

const visible = computed({
  get: () => props.modelValue,
  set: val => emit('update:modelValue', val)
});

let successTimer: number | undefined;
watch(visible, val => {
  if (!val) {
    saveSuccess.value = false;
    if (successTimer) {
      window.clearTimeout(successTimer);
      successTimer = undefined;
    }
  }
});

watch(
  () => props.device,
  val => {
    if (val) {
      Object.assign(form, val);
    } else {
      Object.assign(form, { deviceId: '', name: '', firmwareVersion: '' });
    }
  },
  { immediate: true }
);

const rules = {
  deviceId: [{ required: true, message: '请输入设备ID', trigger: 'blur' }],
  name: [{ required: true, message: '请输入名称', trigger: 'blur' }]
};

async function submit() {
  if (saving.value || saveSuccess.value) return;
  saving.value = true;
  saveSuccess.value = false;
  try {
    if (props.device?.id) {
      await store.update(props.device.id, form);
    } else {
      await store.create(form);
    }
    saveSuccess.value = true;
    successTimer = window.setTimeout(() => {
      emit('submitted');
      visible.value = false;
      saveSuccess.value = false;
      successTimer = undefined;
    }, 500);
  } finally {
    saving.value = false;
  }
}
</script>

<style scoped>
.anim-pulse {
  background: var(--success) !important;
  border-color: var(--success) !important;
}
</style>
