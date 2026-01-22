import { defineStore } from 'pinia';
import { listDevices, getDevice, createDevice, updateDevice, deleteDevice, provisionDevice } from '@/api/device';
import type { Device, DeviceCredential, Paginated } from '@/types/models';

interface DeviceState {
  items: Device[];
  total: number;
  loading: boolean;
  current?: Device;
  credential?: DeviceCredential;
}

export const useDeviceStore = defineStore('device', {
  state: (): DeviceState => ({
    items: [],
    total: 0,
    loading: false,
    current: undefined,
    credential: undefined
  }),
  actions: {
    async fetchList(params?: { page?: number; pageSize?: number; keyword?: string }) {
      this.loading = true;
      try {
        const { items, total } = (await listDevices(params)) as Paginated<Device>;
        this.items = items;
        this.total = total;
      } finally {
        this.loading = false;
      }
    },
    async fetchOne(deviceId: string) {
      this.current = await getDevice(deviceId);
      return this.current;
    },
    async create(payload: Partial<Device>) {
      const created = await createDevice(payload);
      this.items.unshift(created);
      return created;
    },
    async update(deviceId: string, payload: Partial<Device>) {
      const updated = await updateDevice(deviceId, payload);
      this.current = updated;
      this.items = this.items.map(d => (d.deviceId === deviceId ? updated : d));
      return updated;
    },
    async remove(deviceId: string) {
      await deleteDevice(deviceId);
      this.items = this.items.filter(d => d.deviceId !== deviceId);
    },
    async provision(deviceId: string) {
      this.credential = await provisionDevice(deviceId);
      return this.credential;
    }
  }
});
