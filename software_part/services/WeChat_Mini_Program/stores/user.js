import { createUser, deleteUser, listUsers } from '../api/user.js'
import { defineStore } from './pinia.js'

export const useUserStore = defineStore('user', {
  state: () => ({
    items: [],
    total: 0,
    page: 1,
    pageSize: 20,
    loading: false,
  }),
  actions: {
    async fetchList(page = 1, pageSize = 20) {
      this.loading = true
      try {
        const resp = await listUsers(page, pageSize)
        this.items = resp?.items || []
        this.total = resp?.total || 0
        this.page = page
        this.pageSize = pageSize
        return resp
      } finally {
        this.loading = false
      }
    },

    async create(payload) {
      const resp = await createUser(payload)
      return resp
    },

    async remove(id) {
      await deleteUser(id)
      this.items = (this.items || []).filter((u) => u?.id !== id)
      this.total = Math.max(0, (this.total || 0) - 1)
    },
  },
})

