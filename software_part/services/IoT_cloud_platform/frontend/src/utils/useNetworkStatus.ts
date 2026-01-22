import { ref, onMounted, onUnmounted } from 'vue';
import { ElMessage } from 'element-plus';

export function useNetworkStatus() {
  const isOnline = ref(navigator.onLine);
  const showOfflineWarning = ref(false);

  function handleOnline() {
    isOnline.value = true;
    showOfflineWarning.value = false;
    ElMessage.success('网络连接已恢复');
  }

  function handleOffline() {
    isOnline.value = false;
    showOfflineWarning.value = true;
    ElMessage.warning({
      message: '网络连接已断开，请检查您的网络设置',
      duration: 0,
      showClose: true
    });
  }

  onMounted(() => {
    window.addEventListener('online', handleOnline);
    window.addEventListener('offline', handleOffline);
  });

  onUnmounted(() => {
    window.removeEventListener('online', handleOnline);
    window.removeEventListener('offline', handleOffline);
  });

  return {
    isOnline,
    showOfflineWarning
  };
}
