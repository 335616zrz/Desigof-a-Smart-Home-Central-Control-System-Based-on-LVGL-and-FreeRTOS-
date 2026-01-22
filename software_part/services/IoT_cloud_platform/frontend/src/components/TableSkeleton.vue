<template>
  <el-skeleton :rows="rows" animated :loading="true">
    <template #template>
      <div v-for="i in rows" :key="i" style="display: flex; align-items: center; padding: 12px 0;">
        <el-skeleton-item variant="circle" style="width: 40px; height: 40px; margin-right: 16px;" />
        <div style="flex: 1;">
          <el-skeleton-item variant="text" style="width: 60%; margin-bottom: 8px;" />
          <el-skeleton-item variant="text" style="width: 40%;" />
        </div>
      </div>
    </template>
  </el-skeleton>
</template>

<script setup lang="ts">
withDefaults(defineProps<{ rows?: number }>(), {
  rows: 5
});
</script>
