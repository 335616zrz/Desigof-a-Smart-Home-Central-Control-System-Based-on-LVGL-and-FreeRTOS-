<template>
  <el-card shadow="never" class="skeleton-card">
    <el-skeleton :rows="3" animated :loading="true">
      <template #template>
        <el-skeleton-item variant="h3" style="width: 50%; margin-bottom: 16px;" />
        <el-skeleton-item variant="text" style="margin-bottom: 8px;" />
        <el-skeleton-item variant="text" style="width: 80%;" />
      </template>
    </el-skeleton>
  </el-card>
</template>

<style scoped>
.skeleton-card {
  height: 100%;
  min-height: 200px;
}
</style>
