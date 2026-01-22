import { defineStore } from 'pinia';
import { listUsers, createUser, deleteUser, batchCreate, batchDelete, upgradeUser, downgradeUser } from '@/api/user';
import type { User, Paginated } from '@/types/models';

interface UserState {
  items: User[];
  total: number;
  loading: boolean;
}

export const useUserStore = defineStore('user', {
  state: (): UserState => ({ items: [], total: 0, loading: false }),
  actions: {
    async fetchList(params?: { page?: number; pageSize?: number }) {
      this.loading = true;
      try {
        const { items, total } = (await listUsers(params)) as Paginated<User>;
        this.items = items;
        this.total = total;
      } catch (error: any) {
        console.error('Failed to fetch users:', error);
        // If 403, token might be expired - rethrow to let caller handle
        if (error?.response?.status === 403) {
          throw error;
        }
        // For other errors, set empty state
        this.items = [];
        this.total = 0;
      } finally {
        this.loading = false;
      }
    },
    async create(payload: { username: string; password: string; email?: string; role: string }) {
      await createUser(payload);
      // Refetch to ensure consistency with backend ordering
      await this.fetchList({ page: 1, pageSize: 100 });
    },
    async batchCreate(users: Array<{ username: string; password: string; email?: string; role: string }>) {
      await batchCreate(users);
      // Refetch to ensure consistency with backend ordering
      await this.fetchList({ page: 1, pageSize: 100 });
    },
    async remove(id: number) {
      await deleteUser(String(id));
      // Refetch to ensure consistency
      await this.fetchList({ page: 1, pageSize: 100 });
    },
    async removeBatch(ids: number[]) {
      if (!ids.length) return;
      await batchDelete(ids);
      // Refetch to ensure consistency
      await this.fetchList({ page: 1, pageSize: 100 });
    },
    async upgrade(id: number) {
      await upgradeUser(id);
      // Refetch to ensure consistency
      await this.fetchList({ page: 1, pageSize: 100 });
    },
    async downgrade(id: number) {
      await downgradeUser(id);
      // Refetch to ensure consistency
      await this.fetchList({ page: 1, pageSize: 100 });
    }
  }
});
