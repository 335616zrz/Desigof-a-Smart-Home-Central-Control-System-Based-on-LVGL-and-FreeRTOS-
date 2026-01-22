<template>
  <el-dialog v-model="visible" title="设备凭证" width="480px">
    <div v-if="credential" class="grid">
      <div class="item"><span>客户端ID</span><code>{{ clientId }}</code></div>
      <div class="item"><span>用户名</span><code>{{ username }}</code></div>
      <div class="item"><span>密码</span><code>{{ password }}</code></div>
      <div class="item"><span>过期时间</span><code>{{ credential.expiresAt || '无' }}</code></div>
    </div>
    <template #footer>
      <el-button @click="visible = false">关闭</el-button>
      <el-button type="primary" @click="download">下载 JSON</el-button>
    </template>
  </el-dialog>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import type { DeviceCredential } from '@/types/models';

const props = defineProps<{ modelValue: boolean; credential?: DeviceCredential | null }>();
const emit = defineEmits(['update:modelValue']);

const visible = computed({
  get: () => props.modelValue,
  set: val => emit('update:modelValue', val)
});

const clientId = computed(() => props.credential?.clientId || props.credential?.deviceId || '');
const username = computed(() => props.credential?.username || props.credential?.credentialKey || '');
const password = computed(() => props.credential?.password || props.credential?.credentialSecret || '');

function download() {
  if (!props.credential) return;
  const blob = new Blob([JSON.stringify(props.credential, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = 'credential.json';
  a.click();
  URL.revokeObjectURL(url);
}
</script>

<style scoped>
.grid {
  display: grid;
  gap: 10px;
}
.item {
  display: flex;
  flex-direction: column;
  gap: 4px;
}
code {
  background: #f5f7fb;
  padding: 6px 8px;
  border-radius: 6px;
  font-family: 'SFMono-Regular', monospace;
}
</style>
