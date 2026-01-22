import { defineStore } from 'pinia';
import { showAlertNotification, showDeviceStatusNotification } from '@/utils/notifications';

export type EventType = 'telemetry' | 'alert' | 'system';

export interface EventItem {
  id: string;
  type: EventType;
  message: string;
  deviceId?: string;
  level?: string;
  payload?: any;
  timestamp: number;
}

const MAX_EVENTS = 200;

export const useEventStore = defineStore('event', {
  state: () => ({
    items: [] as EventItem[],
    notificationsEnabled: true
  }),
  actions: {
    push(event: EventItem) {
      this.items.unshift(event);
      if (this.items.length > MAX_EVENTS) {
        this.items.splice(MAX_EVENTS);
      }

      // 显示通知
      if (this.notificationsEnabled) {
        if (event.type === 'alert' && event.payload) {
          showAlertNotification(event.payload);
        } else if (event.type === 'system' && event.payload?.status) {
          showDeviceStatusNotification(event.deviceId || 'Unknown', event.payload.status);
        }
      }
    },
    clear() {
      this.items = [];
    },
    toggleNotifications() {
      this.notificationsEnabled = !this.notificationsEnabled;
    }
  }
});
