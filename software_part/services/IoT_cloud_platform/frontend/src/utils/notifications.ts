import { ElNotification } from 'element-plus';
import type { Alert } from '@/types/models';

export function showAlertNotification(alert: Alert) {
  const severityMap: Record<string, 'success' | 'warning' | 'error' | 'info'> = {
    critical: 'error',
    high: 'warning',
    medium: 'warning',
    low: 'info'
  };

  ElNotification({
    title: `新告警: ${alert.deviceId}`,
    message: alert.message || '设备触发告警',
    type: severityMap[alert.severity] || 'warning',
    duration: 5000,
    position: 'top-right',
    showClose: true
  });
}

export function showDeviceStatusNotification(deviceId: string, status: string) {
  const isOnline = status === 'ONLINE';
  ElNotification({
    title: `设备${isOnline ? '上线' : '离线'}`,
    message: `设备 ${deviceId} 已${isOnline ? '上线' : '离线'}`,
    type: isOnline ? 'success' : 'warning',
    duration: 3000,
    position: 'top-right'
  });
}
