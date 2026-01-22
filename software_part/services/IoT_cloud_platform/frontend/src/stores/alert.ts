import { defineStore } from 'pinia';
import { listAlerts, acknowledgeAlert, closeAlert, getAlert } from '@/api/alert';
import { useEventStore } from './event';
import type { AlertItem, Paginated } from '@/types/models';

interface AlertState {
  items: AlertItem[];
  total: number;
  loading: boolean;
  current?: AlertItem;
  currentLoading: boolean;
}

export const useAlertStore = defineStore('alert', {
  state: (): AlertState => ({
    items: [],
    total: 0,
    loading: false,
    current: undefined,
    currentLoading: false
  }),
  actions: {
    async fetchList(params?: { page?: number; pageSize?: number; status?: string; level?: string }) {
      this.loading = true;
      try {
        const result = await listAlerts(params);
        if (result && typeof result === 'object' && 'items' in result) {
          this.items = result.items || [];
          this.total = result.total || 0;
        } else {
          // Fallback if result doesn't match expected format
          this.items = [];
          this.total = 0;
        }
      } catch (error) {
        console.error('Failed to fetch alerts:', error);
        this.items = [];
        this.total = 0;
      } finally {
        this.loading = false;
      }
    },
    async fetchOne(id: number) {
      this.currentLoading = true;
      try {
        this.current = await getAlert(id);
        return this.current;
      } finally {
        this.currentLoading = false;
      }
    },
    async acknowledge(id: number) {
      await acknowledgeAlert(id);
      this.items = this.items.map(item => (item.id === id ? { ...item, status: 'ack' } : item));
      if (this.current?.id === id) this.current = { ...this.current, status: 'ack' };
      const events = useEventStore();
      events.push({
        id: `alert-ack-${id}-${Date.now()}`,
        type: 'alert',
        message: `告警 ${id} 已确认`,
        timestamp: Date.now()
      });
    },
    async close(id: number) {
      await closeAlert(id);
      this.items = this.items.map(item => (item.id === id ? { ...item, status: 'closed' } : item));
      if (this.current?.id === id) this.current = { ...this.current, status: 'closed' };
      const events = useEventStore();
      events.push({
        id: `alert-close-${id}-${Date.now()}`,
        type: 'alert',
        message: `告警 ${id} 已关闭`,
        timestamp: Date.now()
      });
    }
  }
});
