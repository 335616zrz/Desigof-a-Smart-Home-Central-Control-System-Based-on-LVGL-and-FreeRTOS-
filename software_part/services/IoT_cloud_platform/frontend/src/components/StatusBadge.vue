<template>
  <el-tag :type="type" size="small">{{ label }}</el-tag>
</template>

<script setup lang="ts">
const props = defineProps<{ status?: string }>();

const map: Record<string, { type: 'success' | 'danger' | 'info'; label: string }> = {
  ONLINE: { type: 'success', label: '在线' },
  OFFLINE: { type: 'danger', label: '离线' },
  UNKNOWN: { type: 'info', label: '未知' }
};

const type = map[props.status || 'UNKNOWN']?.type || 'info';
const label = map[props.status || 'UNKNOWN']?.label || 'Unknown';
</script>
