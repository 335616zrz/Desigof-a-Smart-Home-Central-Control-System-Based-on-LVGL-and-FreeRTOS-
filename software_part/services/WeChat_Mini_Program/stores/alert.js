import { acknowledgeAlert, closeAlert, getAlert, listAlerts } from '../api/alert.js'
import { defineStore } from './pinia.js'

export const useAlertStore = defineStore('alert', {
  state: () => ({
    items: [],
    total: 0,
    page: 0,
    pageSize: 20,
    loading: false,
    current: null,
  }),
  actions: {
    async fetchList(params) {
      this.loading = true
      try {
        const resp = await listAlerts(params)
        this.items = resp?.items || []
        this.total = resp?.total || 0
        this.page = resp?.page || 0
        this.pageSize = resp?.pageSize || 20
        return resp
      } finally {
        this.loading = false
      }
    },

    async fetchDetail(id) {
      const resp = await getAlert(id)
      this.current = resp || null
      return this.current
    },

    async acknowledge(id) {
      await acknowledgeAlert(id)
      const idx = (this.items || []).findIndex((a) => a?.id === id)
      if (idx >= 0) this.items[idx].status = 'ack'
      if (this.current && this.current.id === id) this.current.status = 'ack'
    },

    async close(id) {
      await closeAlert(id)
      const idx = (this.items || []).findIndex((a) => a?.id === id)
      if (idx >= 0) this.items[idx].status = 'closed'
      if (this.current && this.current.id === id) this.current.status = 'closed'
    },
  },
})

