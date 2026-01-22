<template>
  <div class="badge">
    <el-tag :type="levelType(level)" effect="dark" size="small">{{ levelLabel(level) }}</el-tag>
    <el-tag :type="statusType(status)" size="small" effect="plain">{{ statusLabel(status) }}</el-tag>
  </div>
</template>

<script setup lang="ts">
defineProps<{
  level: 'critical' | 'warning' | 'info' | string;
  status: 'open' | 'ack' | 'closed' | string;
}>();

function levelType(level: string) {
  if (level === 'critical') return 'danger';
  if (level === 'warning') return 'warning';
  return 'info';
}
function levelLabel(level: string) {
  if (level === 'critical') return '严重';
  if (level === 'warning') return '警告';
  return '提示';
}
function statusType(status: string) {
  if (status === 'open') return 'danger';
  if (status === 'ack') return 'warning';
  return 'success';
}
function statusLabel(status: string) {
  if (status === 'open') return '未处理';
  if (status === 'ack') return '已确认';
  return '已关闭';
}
</script>

<style scoped>
.badge {
  display: flex;
  gap: 6px;
  align-items: center;
}
</style>
