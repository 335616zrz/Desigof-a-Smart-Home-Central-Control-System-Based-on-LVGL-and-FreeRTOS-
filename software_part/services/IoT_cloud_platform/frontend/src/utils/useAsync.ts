import { ref } from 'vue';
import { ElMessage } from 'element-plus';

interface UseAsyncOptions {
  successMessage?: string;
  errorMessage?: string;
  showSuccessMessage?: boolean;
  showErrorMessage?: boolean;
}

export function useAsync<T = any>(
  asyncFn: (...args: any[]) => Promise<T>,
  options: UseAsyncOptions = {}
) {
  const {
    successMessage,
    errorMessage = '操作失败',
    showSuccessMessage = false,
    showErrorMessage = true
  } = options;

  const loading = ref(false);
  const error = ref<Error | null>(null);
  const data = ref<T | null>(null);

  async function execute(...args: any[]): Promise<T | null> {
    loading.value = true;
    error.value = null;

    try {
      const result = await asyncFn(...args);
      data.value = result;

      if (showSuccessMessage && successMessage) {
        ElMessage.success(successMessage);
      }

      return result;
    } catch (err: any) {
      error.value = err;

      if (showErrorMessage) {
        const msg = err.response?.data?.message || err.message || errorMessage;
        ElMessage.error(msg);
      }

      return null;
    } finally {
      loading.value = false;
    }
  }

  function reset() {
    loading.value = false;
    error.value = null;
    data.value = null;
  }

  return {
    loading,
    error,
    data,
    execute,
    reset
  };
}
