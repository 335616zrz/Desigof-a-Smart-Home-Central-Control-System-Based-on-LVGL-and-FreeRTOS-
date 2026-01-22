import { describe, it, expect, vi, beforeEach } from 'vitest';
import { setActivePinia, createPinia } from 'pinia';
import { useAlertStore } from './alert';

vi.mock('@/api/alert', () => ({
  listAlerts: vi.fn(async () => ({ items: [{ id: 1, deviceId: 'd1', level: 'warning', message: 'hi', status: 'open', createdAt: Date.now() }], total: 1 })),
  acknowledgeAlert: vi.fn(async () => {}),
  closeAlert: vi.fn(async () => {}),
  getAlert: vi.fn(async (id: number) => ({ id, deviceId: 'd1', level: 'warning', message: 'hi', status: 'open', createdAt: Date.now() }))
}));

// mock event store to avoid side effects
vi.mock('./event', () => ({
  useEventStore: () => ({
    push: vi.fn()
  })
}));

describe('alert store', () => {
  beforeEach(() => {
    setActivePinia(createPinia());
  });

  it('fetches list', async () => {
    const store = useAlertStore();
    await store.fetchList();
    expect(store.items.length).toBe(1);
    expect(store.total).toBe(1);
  });

  it('acknowledges alert status', async () => {
    const store = useAlertStore();
    store.items = [{ id: 1, deviceId: 'd1', level: 'warning', message: 'hi', status: 'open', createdAt: Date.now() }];
    await store.acknowledge(1);
    expect(store.items[0].status).toBe('ack');
  });
});
