<template>
  <div class="error-boundary">
    <el-result
      v-if="hasError"
      icon="error"
      title="出错了"
      sub-title="应用遇到了一个意外错误"
    >
      <template #extra>
        <el-button type="primary" @click="handleReset">重新加载</el-button>
        <el-button @click="handleBackHome">返回首页</el-button>
      </template>
      <template #default>
        <el-descriptions v-if="errorInfo" title="错误详情" :column="1" border>
          <el-descriptions-item label="错误消息">{{ errorInfo.message }}</el-descriptions-item>
          <el-descriptions-item v-if="errorInfo.stack" label="堆栈">
            <pre style="max-height: 200px; overflow: auto; font-size: 12px;">{{ errorInfo.stack }}</pre>
          </el-descriptions-item>
        </el-descriptions>
      </template>
    </el-result>
    <slot v-else></slot>
  </div>
</template>

<script setup lang="ts">
import { ref, onErrorCaptured } from 'vue';
import { useRouter } from 'vue-router';

const router = useRouter();
const hasError = ref(false);
const errorInfo = ref<{ message: string; stack?: string } | null>(null);

onErrorCaptured((err: any) => {
  hasError.value = true;
  errorInfo.value = {
    message: err.message || '未知错误',
    stack: err.stack
  };
  console.error('Error captured by boundary:', err);
  return false; // 阻止错误继续向上传播
});

function handleReset() {
  hasError.value = false;
  errorInfo.value = null;
  window.location.reload();
}

function handleBackHome() {
  hasError.value = false;
  errorInfo.value = null;
  router.push('/');
}
</script>

<style scoped>
.error-boundary {
  min-height: 400px;
  display: flex;
  align-items: center;
  justify-content: center;
}
</style>
